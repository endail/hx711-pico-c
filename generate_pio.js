
import { PIOAssembler } from "pioasm";
var fs = require("fs");

const source = fs.readFileSync(
    "src/hx711_noblock.pio",
    "utf8");

const pioasm = new PIOAssembler();
    pioasm.assemble(source).then(result => {
    console.log(result.output);
});
