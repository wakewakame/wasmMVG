# refinePose — カメラ姿勢の非線形最適化

`refinePose` は、初期姿勢 $(R_0, t_0)$ から出発し、再投影誤差を最小化することでカメラ姿勢を精密化する関数である。
最適化には **Levenberg-Marquardt (LM) 法** を用い、カメラフレームでのパラメータ化により自由度 (DOF) のマスクを実現している。

## 1. ピンホールカメラモデル

ワールド座標 $P_w \in \mathbb{R}^3$ をカメラ座標系に変換し、画像平面に投影する。

$$
P_c = R P_w + t
$$

ここで $R \in SO(3)$ は回転行列、$t = -RC$（$C$ はカメラ中心のワールド座標）は並進ベクトルである。

カメラ座標 $P_c = (X, Y, Z)^T$ の画像への投影は

$$
u = f \frac{X}{Z} + c_x, \qquad v = f \frac{Y}{Z} + c_y
$$

で与えられる。$f$ は焦点距離、$(c_x, c_y)$ は主点である。

## 2. 最適化問題

$N$ 組の 3D-2D 対応 $\{(P_i, p_i)\}_{i=1}^N$ が与えられたとき、再投影誤差の二乗和を最小化する：

$$
\min_{R, t} \sum_{i=1}^{N} \left\| \pi(R P_i + t) - p_i \right\|^2
$$

ここで $\pi$ はピンホール投影関数である。

## 3. カメラフレームのパラメータ化

姿勢の更新をカメラ座標系で定義する。更新パラメータ $\delta = (\delta\omega, \delta t) \in \mathbb{R}^6$ に対して

$$
R_{\text{new}} = \Delta R \cdot R, \qquad t_{\text{new}} = \Delta R \cdot t + \delta t
$$

と更新する。$\Delta R = \text{rodrigues}(\delta\omega)$ は回転の微小更新である。

この定式化のポイントは、更新がすべてカメラ座標系で記述されるため、各パラメータが画像上の変化と直感的に対応することである。例えば $\delta t_x$ は画像上の水平移動、$\delta\omega_z$ は画像面内の回転に対応する。

## 4. Rodrigues の回転公式

3 次元ベクトル $\omega \in \mathbb{R}^3$ から回転行列への変換：

$$
R(\omega) = I + \sin\theta \cdot K + (1 - \cos\theta) K^2
$$

ここで $\theta = \|\omega\|$ は回転角、$k = \omega / \theta$ は回転軸の単位ベクトル、$K = [k]_\times$ は歪対称行列：

$$
K = \begin{pmatrix} 0 & -k_z & k_y \\ k_z & 0 & -k_x \\ -k_y & k_x & 0 \end{pmatrix}
$$

$\theta \to 0$ では $R \to I$（恒等行列）に退化する。

## 5. ヤコビアンの導出

残差 $r_i = \pi(P_{c,i}) - p_i$ の更新パラメータ $\delta = (\delta\omega, \delta t)$ に対するヤコビアンは、連鎖律により次のように分解される：

$$
J_i = \frac{\partial r_i}{\partial \delta} = \frac{\partial \pi}{\partial P_c} \cdot \frac{\partial P_c}{\partial \delta}
$$

### 5.1 投影のヤコビアン

$\pi(P_c) = \left( f\frac{X}{Z} + c_x,\ f\frac{Y}{Z} + c_y \right)$ に対して

$$
\frac{\partial \pi}{\partial P_c} = \begin{pmatrix} \frac{f}{Z} & 0 & -\frac{fX}{Z^2} \\ 0 & \frac{f}{Z} & -\frac{fY}{Z^2} \end{pmatrix}
$$

### 5.2 カメラ座標の回転に対する微分

カメラフレーム更新 $P_c' = \Delta R \cdot P_c$ を $\delta\omega$ で微分する。$\delta\omega \to 0$ の近傍では

$$
P_c' \approx (I + [\delta\omega]_\times) P_c = P_c + \delta\omega \times P_c
$$

であるから

$$
\frac{\partial P_c}{\partial \delta\omega} = -[P_c]_\times = \begin{pmatrix} 0 & Z & -Y \\ -Z & 0 & X \\ Y & -X & 0 \end{pmatrix}
$$

（外積 $\delta\omega \times P_c = -[P_c]_\times \delta\omega = [P_c]_\times^T \delta\omega$ の関係を利用）

### 5.3 並進に対する微分

$P_c' = P_c + \delta t$ より

$$
\frac{\partial P_c}{\partial \delta t} = I_{3 \times 3}
$$

### 5.4 完全なヤコビアン

以上をまとめると、各点 $i$ に対する $2 \times 6$ ヤコビアンは

$$
J_i = \frac{\partial \pi}{\partial P_c} \begin{pmatrix} [P_c]_\times^T & I \end{pmatrix}
$$

パラメータの並び順は $(\delta\omega_x, \delta\omega_y, \delta\omega_z, \delta t_x, \delta t_y, \delta t_z)$ である。

## 6. Levenberg-Marquardt 法

全残差ベクトル $r \in \mathbb{R}^{2N}$ と全ヤコビアン $J \in \mathbb{R}^{2N \times d}$（$d$ は有効自由度数）を構築し、以下の正規方程式を解く：

$$
\left( J^T J + \lambda \cdot \text{diag}(J^T J) \right) \delta = -J^T r
$$

- $\lambda$ が小さいとき：ガウス-ニュートン法に近づき、二次収束が期待できる
- $\lambda$ が大きいとき：最急降下法に近づき、安定だが収束は遅い

更新後のコスト $\|r_{\text{new}}\|^2$ を評価し、コストが減少すれば更新を採用して $\lambda$ を $1/10$ に、増加すれば棄却して $\lambda$ を $10$ 倍する。$\lambda$ は $[10^{-10}, 10^{10}]$ にクランプされる。

## 7. 自由度マスク (DOF Mask)

6 ビットのマスクで最適化する自由度を選択する。ビットは上位から

| bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|:----:|:----:|:----:|:----:|:----:|:----:|
| $t_x$ | $t_y$ | $t_z$ | $\omega_x$ | $\omega_y$ | $\omega_z$ |

と対応する。

実装では、完全な $2 \times 6$ ヤコビアンからマスクに対応する列のみを抽出して縮約ヤコビアン $J \in \mathbb{R}^{2N \times d}$ を構成する。$\delta$ もマスクされた成分のみ計算し、残りの成分はゼロのままにすることで、選択されていない自由度は一切変化しない。

### 利用例

| 条件点数 | マスク | 有効自由度 | 意味 |
|:---:|:---:|:---|:---|
| 1 | `0b110000` | $t_x, t_y$ | 画像面内の平行移動のみ |
| 2 | `0b111001` | $t_x, t_y, t_z, \omega_z$ | 平行移動 + 奥行き + 画面内回転 |
| 3+ | `0b111111` | 全 6 自由度 | 完全な姿勢推定 |

## 8. 収束条件と安全対策

- **収束判定**: $\|\delta\| < 10^{-10}$ でパラメータが十分小さくなれば打ち切り
- **最大反復**: 既定は 20 回
- **カメラ背面ガード**: $Z_c \le 0$ の点が存在した場合、反復を中断して現在の最良解を返す
