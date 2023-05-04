export type Module = {
  hello: (name: string) => string,
};
declare function wasmMVG(): Promise<Module>;
export default wasmMVG;
