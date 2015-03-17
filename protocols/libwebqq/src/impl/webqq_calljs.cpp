
#include <memory>
#include <jsapi.h>
#ifndef MOZJS_MAJOR_VERSION
#include <jsversion.h>
#endif
#include <QtCore>

#include "webqq_calljs.hpp"

static JSClass global_class = {
	"global",
	JSCLASS_GLOBAL_FLAGS|JSCLASS_NEW_RESOLVE,
	JS_PropertyStub,
#ifdef MOZJS_MAJOR_VERSION
	JS_DeletePropertyStub,
#else
	JS_PropertyStub,
#endif
	JS_PropertyStub,
	JS_StrictPropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	NULL,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

std::string webqq::qqimpl::call_js_helper_function_in_buffer(const char* js_content, int js_content_length, std::string helper_file, std::string function, std::vector< std::string > args)
{
#if MOZJS_MAJOR_VERSION >= 24
	std::shared_ptr<JSRuntime> jsrt{ JS_NewRuntime(1024*1024*10, JS_NO_HELPER_THREADS) , JS_DestroyRuntime};
#else
	std::shared_ptr<JSRuntime> jsrt{ JS_NewRuntime(1024*1024*10) , JS_DestroyRuntime};
#endif
	std::shared_ptr<JSContext> jsctx{ JS_NewContext(jsrt.get(), 16*1024) , JS_DestroyContext};

	JS_SetOptions(jsctx.get(), JS_GetOptions(jsctx.get()) |JSOPTION_VAROBJFIX|JSOPTION_COMPILE_N_GO|JSOPTION_NO_SCRIPT_RVAL);

#ifdef 	MOZJS_MAJOR_VERSION
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);

	JSObject* global_object = JS_NewGlobalObject(jsctx.get(), &global_class, NULL, options);
    JSAutoCompartment ac(jsctx.get(), global_object);
#elif JS_VERSION == 185
	JSObject* global_object = JS_NewCompartmentAndGlobalObject(jsctx.get(), &global_class, NULL);
#else
	JSObject* global_object = JS_NewGlobalObject(jsctx.get(), &global_class, NULL);
#endif

	JS_SetGlobalObject(jsctx.get(), global_object);

	JS_InitStandardClasses(jsctx.get(), global_object);

#if JS_VERSION == 185
	JSObject * script
#else
	JSScript * script
#endif
	= JS_CompileScript(jsctx.get(), global_object, js_content, js_content_length,  helper_file.c_str(), 0);

	JS_ExecuteScript(jsctx.get(), global_object, script, NULL);

	jsval res;
	std::vector<jsval> argv;
	char* res_;

	for (auto _a : args)
	{
		JSString* js_arg = JS_NewStringCopyN(jsctx.get(), _a.c_str(), _a.length());
		argv.push_back(STRING_TO_JSVAL(js_arg));
	}

	JS_CallFunctionName(jsctx.get(), global_object, function.c_str(), argv.size(), argv.data(), &res);

	res_ = JS_EncodeString(jsctx.get(),JSVAL_TO_STRING(res));
	std::string ret = res_;
	JS_free(jsctx.get(),res_);
	return ret;

}


std::string webqq::qqimpl::call_js_helper_function(std::string helper_file, std::string function, std::vector< std::string > args)
{
	QFile helperfile(helper_file.c_str());

	helperfile.open(QIODevice::ReadOnly);

	auto js_content = helperfile.readAll();

	return call_js_helper_function_in_buffer(js_content.constData(), js_content.length(), helper_file, function, args);

}

