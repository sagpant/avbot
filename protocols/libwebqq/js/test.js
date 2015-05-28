var fs = require("fs")
var vm = require('vm')

var content = fs.readFileSync("./compat.js")
vm.runInThisContext(content)

var content = fs.readFileSync("./comm.js")
vm.runInThisContext(content)

loginform = {
	ptqrlogin : "1",
	u : "464893490",
	p : "552164545612"
}

TEA = window.TEA;

console.log(getSubmitUrl("login"));
