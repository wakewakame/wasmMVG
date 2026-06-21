import { pages } from 'virtual:page-list'

// const desc = (
//   <div>
//     <p>Example Pages</p>
//   </div>
// );
const desc = (() => {
  const div = document.createElement('div');
  const p = document.createElement('p');
  p.textContent = 'Example Pages';
  div.appendChild(p);
  return div;
})();

// const examples = (
//   <ul>
//     <li><a href="./pages/foo.html">foo</a></li>
//     <li><a href="./pages/bar.html">bar</a></li>
//     ...
//   </ul>
// );
const examples = (() => {
  const ul = document.createElement('ul');
  const list = document.createDocumentFragment();
  for (const page of pages) {
    const li = document.createElement('li');
    const link = document.createElement('a');
    link.href = `./pages/${page}`;
    link.textContent = page.replace(/\.html$/, '');
    li.appendChild(link);
    list.appendChild(li);
  }
  ul.appendChild(list);
  return ul;
})();

// <div id="app">
//  <desc />
//  <examples />
// </div>
const app = document.getElementById('app') as HTMLDivElement;
app.appendChild(desc);
app.appendChild(examples);
