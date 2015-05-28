
avbotuserAgent = function(){

	function toLowerCase(){}

	return {
		toLowerCase : toLowerCase
	}

}();

navigator = {appName:"Netscape", userAgent: avbotuserAgent };

g_appid = 501004106;

g_href = "";

g_domain = "qq.com";

g_begTime = new Date;

$ = {}

window = {  $ : {} };

document = {};

document.loginform = {};

document.loginform.regmaster = {};

loginform = [
	{ name : "u" , value : "2082029534" },
	{ name : "p" , value : "txwqnmlgb" }
];


loginform.p = {};

loginform.p.value ="txwqnmlgb";
loginform.verifycode = { value : "abcd" };

document.forms = [ loginform ];


document.getElementById = function(id){
	return {};
}

document.cookie = {};

document.cookie.match = function(regex){}
