
var PIOAssembler = require("pioasm");
var fs = require("fs");

console.log(PIOAssembler);

const source = fs.readFileSync(
"src/hx711_noblock.pio",
"utf8");

const pioasm = new PIOAssembler();
    pioasm.assemble(source).then(result => {
    console.log(result.output);
});
