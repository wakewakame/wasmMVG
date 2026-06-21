import { defineConfig } from 'vite'
import { readdirSync } from 'node:fs'
import { resolve } from 'node:path'
import { exactRegex } from '@rolldown/pluginutils'

// ./pages/*.html の一覧を取得
// const pages = ['foo.html', 'bar.html', ...];
const pages = readdirSync('./pages').filter(file => file.endsWith('.html'));

// ./src/*.ts から pages を参照できるようにする
// `import { pages } from 'virtual:page-list'` のような形で利用可能
const pageListPlugin = () => {
  const virtualModuleId = 'virtual:page-list';
  const resolvedVirtualModuleId = '\0' + virtualModuleId;
  return {
    name: 'page-list',
    resolveId: {
      filter: { id: exactRegex(virtualModuleId) },
      handler() { return resolvedVirtualModuleId; },
    },
    load: {
      filter: { id: exactRegex(resolvedVirtualModuleId) },
      handler() { return `export const pages = ${JSON.stringify(pages)};`; },
    },
  }
};

// const input = { foo: '/path/to/pages/foo.html', bar: '/path/to/pages/bar.html', ... };
const input = Object.fromEntries(
  pages.map(page => [
    page.replace(/\.html$/, ''),
    resolve(import.meta.dirname, 'pages', page),
  ])
);

// Vite の設定
export default defineConfig({
  build: {
    rolldownOptions: {
      input: {
        main: resolve(import.meta.dirname, 'index.html'),
        ...input,
      },
    },
  },
  plugins: [pageListPlugin()],
});
