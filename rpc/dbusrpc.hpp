
#pragma once

#include <QObject>
#include <QString>
#include <QList>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;

struct SearchResultData
{
	QString data;
	QString channel;
	QString nick;
	QString message;
	QString id;
};

struct SearchResult
{
    int resoult_count;
    QList<SearchResultData> data;
	double time_used;
};

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(SearchResultData)

#include "../libavbot/avchannel.hpp"

class DBusRPC : public QObject
{
    Q_OBJECT

public:
	typedef boost::signals2::signal <
		void( channel_identifier, avbotmsg )
	> on_message_signal_type;

	DBusRPC(boost::asio::io_service& io, on_message_signal_type & on_message, std::function<void(
		std::string,
		std::string,
		std::string,
		std::function<void (boost::system::error_code, pt::wptree)>
	)> do_search_func);

Q_SIGNALS:
	void quit();

public Q_SLOTS:

	SearchResult search(QString searchstring);

private:
	std::function<void(
		std::string,
		std::string,
		std::string,
		std::function<void (boost::system::error_code, pt::wptree)>
	)> do_search;
};

