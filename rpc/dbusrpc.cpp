
#include <QtDBus>
#include <QtDBus/QDBusConnection>

#include "dbusrpc.hpp"
#include "avbotrpc_adaptor.h"
#include <QCoreApplication>

// Marshall the SearchResultData data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const SearchResultData &mystruct)
{
    argument.beginStructure();
    argument << mystruct.data << mystruct.channel << mystruct.nick << mystruct.message << mystruct.id;
    argument.endStructure();
    return argument;
}

// Marshall the SearchResultData data into a D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, SearchResultData &mystruct)
{
    argument.beginStructure();
    argument >> mystruct.data >> mystruct.channel >> mystruct.nick >> mystruct.message >> mystruct.id;
    argument.endStructure();
    return argument;
}

// Marshall the SearchResult data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const SearchResult &mystruct)
{
    argument.beginStructure();
    argument << mystruct.resoult_count << mystruct.data << mystruct.time_used;
    argument.endStructure();
    return argument;
}

// Retrieve the SearchResult data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, SearchResult &mystruct)
{
    argument.beginStructure();
    argument >> mystruct.resoult_count >> mystruct.data >> mystruct.time_used;
    argument.endStructure();
    return argument;
}

DBusRPC::DBusRPC(boost::asio::io_service& io, on_message_signal_type & on_message, std::function<void(std::string, std::string, std::string, std::function<void (boost::system::error_code, pt::wptree)>)> do_search_func)
	: QObject()
{
	qDBusRegisterMetaType<SearchResultData>();
	qDBusRegisterMetaType<SearchResult>();

	new AvbotAdaptor(this);
	QDBusConnection::sessionBus().registerObject("/avbotrpc", this);
	QDBusConnection::sessionBus().registerService("org.avplayer.avbot");

	connect(this, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));
}

SearchResult DBusRPC::search(QString searchstring)
{

}


