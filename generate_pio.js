#!/usr/bin/node

import { PIOAssembler } from 'pioasm';

const source = `
  .program blink
  pull block
  out y, 32
`;

const pioasm = new PIOAssembler();
pioasm.assemble(source).then(result => {
  console.log(result.output);
})

/*
var PIOAssembler = import("pioasm");
var fs = require("fs");

console.log(PIOAssembler);

const source = fs.readFileSync(
"src/hx711_noblock.pio",
"utf8");

const pioasm = new PIOAssembler();
    pioasm.assemble(source).then(result => {
    console.log(result.output);
});
*/