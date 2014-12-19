
#include "pythonscriptengine.hpp"
#include "boost/json_parser_write.hpp"
#include <ctime>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/property_tree/json_parser.hpp>
#include <boost/python.hpp>

namespace py = boost::python;

boost::system::error_code no_module;
boost::system::error_code no_handle_func;

bool python_init = false;
bool disable_python = true;

struct MessageSender {
	boost::function<void(std::string)> sender_;

	void send_message(std::string msg) { sender_(msg); }
};

class PythonScriptEngine
{
public:
	PythonScriptEngine(asio::io_service &io, std::string _channel_name, std::function<void(std::string)> sender)
		: io_(io)
		, sender_(sender)
		, channel_name_(_channel_name)
		, file_changed_(true)
	{
		try {
			if (!python_init) {
				Py_Initialize();
				python_init = true;
				module_ = py::import("avbot");
				py::scope s(module_);
				reload();
			}
		}
		catch (...) {
			PyErr_Print();
		}
	}

	~PythonScriptEngine() {}

	void reload() {
		file_changed_ = false;
		global_ = module_.attr("__dict__");
		py::class_<MessageSender>("MessageSender")
			.def("send_message", &MessageSender::send_message);
		py::object pysender = global_["MessageSender"]();
		py::extract<MessageSender &>(pysender)().sender_ = sender_;
		pyhandler_ = global_["MessageHandler"]();
		pyhandler_.attr("send_message") = pysender.attr("send_message");
		disable_python = false;
		last_write_time_ = fs::last_write_time(fs::path("./avbot.py"));
	}

	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		try {
			boost::system::error_code ignore_ec;
			fs::path script_file("./avbot.py");
			std::time_t now_write_time = fs::last_write_time(script_file);
			if (fs::exists(script_file) && now_write_time != last_write_time_) {
				PyImport_ReloadModule(module_.ptr());
				file_changed_ = true;
				reload();
			}
			if (disable_python)
				return;
			std::stringstream ss;
			auto json_msg = av_msg_make_json(cid, msg);
			json_msg.put("channel", channel_name_ );
			boost::property_tree::json_parser::write_json(ss, json_msg);
			pyhandler_.attr("on_message")(ss.str());
		}
		catch (...) {
			PyErr_Print();
		}
	}

private:
	asio::io_service &io_;
	std::string channel_name_;
	std::function<void(std::string)> sender_;
	static py::object module_;
	static py::object global_;
	py::object pyhandler_;
	bool file_changed_;
	std::time_t last_write_time_;
};

py::object PythonScriptEngine::module_;
py::object PythonScriptEngine::global_;

avbot_extension make_python_script_engine(asio::io_service &io, std::string channel_name, std::function<void(std::string)> sender)
{
	return avbot_extension(channel_name, PythonScriptEngine(io, channel_name, sender));
}
