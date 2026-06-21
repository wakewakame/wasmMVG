// vite.config.ts の page-list プラグインが提供する仮想モジュールの型定義。
// 仮想モジュールはディスク上に実体が無いため、tsserver/tsc 向けに手動で宣言する。
declare module 'virtual:page-list' {
  /** ./pages/*.html の一覧 (index.html からの相対パス) */
  export const pages: string[];
}
