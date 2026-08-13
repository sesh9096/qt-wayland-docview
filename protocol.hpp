#include <vector>
#include <QFile>
#include <QTextDocument>
#include <QTextEdit>
// #include <QTextTable>
#include <QXmlStreamReader>
#include <QtCore>

typedef short ProtocolVersion;
#define PROTOCOL_VERSION_FROM_STRING(str) (str.toShort())

QString joinLines(QStringView sv, bool isCopyright = false) {
	// return QString(sv);
	QString ret;
	ret.reserve(sv.length());
	QList<QStringView> parts = sv.split('\n');
	qsizetype i = 1;
	if (isCopyright) {
		for (;i < parts.size(); i++) {
			if (parts[i].length() == 0) break;
			else {
				ret.append(parts[i].trimmed());
				ret.append("\n\n");
			}
		}
	}
	for (; i < parts.size(); i++) {
		ret.append(parts[i].trimmed());
		ret.append('\n');
	}
	ret.shrink_to_fit();
	return ret;
}

typedef struct Description{
	QString summary;
	QString content;
	Description(QXmlStreamReader& reader) {
		summary = QString(reader.attributes().value("summary"));
		content = joinLines(reader.readElementText());
	}
	Description(): summary(QString()), content(QString()) {}
} Description;

typedef struct Entry{
	QString name;
	QString value;
	QString summary;
	ProtocolVersion since;
	ProtocolVersion deprecated_since;
	Description description;

	Entry(QXmlStreamReader& reader) {
		QXmlStreamAttributes attrs = reader.attributes();
		name = QString(attrs.value("name"));
		value = QString(attrs.value("value"));
		summary = QString(attrs.value("summary"));
		since = PROTOCOL_VERSION_FROM_STRING(attrs.value("since"));
		deprecated_since = PROTOCOL_VERSION_FROM_STRING(attrs.value("deprecated-since"));

		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "description") {
					description = Description(reader);
				} else {
					throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				tag = reader.name();
				if(tag == "entry") return;
				else throw std::runtime_error("Unexpected close tag");
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
	}
} Entry;

typedef struct Enum{
	QString name;
	short since;
	bool bitfield;
	Description description;
	std::vector<Entry> entries;

	Enum(QXmlStreamReader& reader) {
		QXmlStreamAttributes attrs = reader.attributes();
		name = QString(attrs.value("name"));
		since = attrs.value("since").toShort();
		bitfield = attrs.value("bitfield") == "true";

		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "description") {
					description = Description(reader);
				} else if (tag == "entry") {
					entries.emplace_back(reader);
				} else {
					throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				tag = reader.name();
				if(tag == "enum") return;
				else throw std::runtime_error("Unexpected close tag");
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
	}
} Enum;

typedef struct Arg{
	QString name;
	QString type;
	QString summary;
	QString interface;
	bool allow_null;
	QString enum_name;
	Description description;
	Arg(QXmlStreamReader& reader) {
		QXmlStreamAttributes attrs = reader.attributes();
		name = QString(attrs.value("name"));
		type = QString(attrs.value("type"));
		summary = QString(attrs.value("summary"));
		interface = QString(attrs.value("interface"));
		allow_null = attrs.value("allow-null") == "true";
		enum_name = QString(attrs.value("enum"));

		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "description") {
					description = Description(reader);
				} else {
					throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				tag = reader.name();
				if(tag == "arg") return;
				else throw std::runtime_error("Unexpected close tag");
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
	}
	void signature(QString &buf) const {
		buf.append(name);
		buf.append(": ");
		if(allow_null) buf.append('?');
		buf.append(type);
		if (interface.length() != 0) {
			buf.append("<");
			buf.append(interface);
			buf.append(">");
		}
		if (enum_name.length() != 0) {
			buf.append("<");
			buf.append(enum_name);
			buf.append(">");
		}
	}
} Arg;

typedef struct Request{
	QString name;
	QString type;
	ProtocolVersion since;
	ProtocolVersion deprecated_since;
	std::vector<Arg> args;
	Description description;
	Request(QXmlStreamReader& reader) {
		QXmlStreamAttributes attrs = reader.attributes();
		name = QString(attrs.value("name"));
		type = QString(attrs.value("type"));
		since = PROTOCOL_VERSION_FROM_STRING(attrs.value("since"));
		deprecated_since = PROTOCOL_VERSION_FROM_STRING(attrs.value("deprecated-since"));

		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "description") {
					description = Description(reader);
				} else if (tag == "arg") {
					args.emplace_back(reader);
				} else {
					throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				tag = reader.name();
				// request and event share a class
				if (tag == "request" || tag == "event") return;
				else throw std::runtime_error("Unexpected close tag");
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
	}

	QString signature() const {
		QString signature = name + "(";
		std::vector<Arg>::const_iterator arg = args.cbegin();
		if (arg != args.cend()) {
			arg->signature(signature);
			arg++;
			while (arg != args.cend()) {
				signature += ", ";
				arg->signature(signature);
				arg++;
			}
		}
		signature += ")";
		return signature;
	}
} Request;

typedef Request Event;

typedef struct Interface{
	Description description;
	QString name;
	ProtocolVersion version;
	bool frozen;
	std::vector<Request> requests;
	std::vector<Event> events;
	std::vector<Enum> enums;

	Interface(QXmlStreamReader& reader) {
		QXmlStreamAttributes attrs = reader.attributes();
		name = QString(attrs.value("name"));
		version = PROTOCOL_VERSION_FROM_STRING(attrs.value("name"));
		frozen = attrs.value("name") == "true";
		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "description") {
					description = Description(reader);
				} else if (tag == "request") {
					requests.emplace_back(reader);
				} else if (tag == "event") {
					events.emplace_back(reader);
				} else if (tag == "enum") {
					enums.emplace_back(reader);
					// } else {
					// 	throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				tag = reader.name();
				if(tag == "interface") return;
				else throw std::runtime_error("Unexpected close tag");
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
	}
} Interface;

typedef struct Protocol{
	Description description;
	QString name;
	QString copyright;
	std::vector<Interface> interfaces;

	void parseProtocol(QXmlStreamReader& reader) {
		name = QString(reader.attributes().value("name"));
		while (!reader.atEnd()) {
			QStringView tag;
			QXmlStreamReader::TokenType token = reader.readNext();
			switch (token) {
			case QXmlStreamReader::StartElement:
				tag = reader.name();
				if (tag == "copyright") {
					copyright = joinLines(reader.readElementText(), true);
				} else if (tag == "description") {
					description = Description(reader);
				} else if (tag == "interface") {
					interfaces.emplace_back(reader);
					// } else {
					// 	throw std::runtime_error("Malformed xml");
				}
				break;
			case QXmlStreamReader::EndElement:
				if(tag == "protocol") return;
			default:
				break;
				// std::cout << xml.tokenType();
			}
		}
		return;
	}
	template<typename T>
	Protocol (T filename) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			throw std::runtime_error("Open File Error");
		QXmlStreamReader reader(&file);
		while (!reader.atEnd()) {
			//... do processing
			// QXmlStreamAttributes attrs;
			switch(reader.readNext()) {
			case QXmlStreamReader::StartElement:
				if (reader.name() == "protocol") {
					parseProtocol(reader);
					return;
				}
				break;
			case QXmlStreamReader::StartDocument:
			case QXmlStreamReader::Comment:
				break;
			default:
				qDebug() << reader.tokenType();
				throw std::runtime_error("Malformed xml");
				break;
			}
		}
		if (reader.hasError()) {
			//... do error handling
		}
		// std::cout << std::endl;
		file.close();
		throw std::runtime_error("Malformed xml");
	}
	void writeDocument(QTextCursor &cursor) const {
		QTextBlockFormat block_format;
		block_format.setBottomMargin(5);
		QTextCharFormat protocol_format;
		protocol_format.setFontPointSize(protocol_format.font().pointSize() + 5);
		protocol_format.setFontWeight(QFont::Bold);
		protocol_format.setAnchorNames({name});
		protocol_format.setAnchorHref("#" + name);
		protocol_format.setAnchor(true);
		QTextCharFormat interface_format;
		interface_format.setFontPointSize(interface_format.font().pointSize() + 2);
		interface_format.setFontWeight(QFont::Bold);
		QTextCharFormat summary_format;
		summary_format.setFontWeight(QFont::Bold);
		QTextCharFormat content_format;
		QFont monospace("monospace");
		QTextCharFormat monospace_format;
		monospace_format.setFont(monospace);
		QTextCharFormat link_format = monospace_format;

		auto insertDescription = [&] (Description const& description) {
			if (!description.summary.isEmpty()) {
				cursor.insertText(description.summary, summary_format);
				cursor.insertBlock();
			}
			if (!description.content.isEmpty()) {
				cursor.insertMarkdown(description.content);
				cursor.insertBlock();
			}
		};
		auto insertArg = [&] (Arg const& arg) {
			cursor.insertText(arg.name);
			cursor.insertText(": ", monospace_format);
			if(arg.allow_null) cursor.insertText("?");
			cursor.insertText(arg.type);
			if (arg.interface.length() != 0) {
				cursor.insertText("<");
				link_format.setAnchor(true);
				link_format.setAnchorHref("#" + arg.interface);
				cursor.insertText(arg.interface, link_format);
				cursor.insertText(">", monospace_format);
			}
			if (arg.enum_name.length() != 0) {
				cursor.insertText("<");
				link_format.setAnchor(true);
				link_format.setAnchorHref("#" + arg.enum_name);
				cursor.insertText(arg.enum_name, link_format);
				cursor.insertText(">", monospace_format);
			}
		};
		auto insertEvent = [&] (Event const& event) {
			link_format.setAnchorNames({event.name});
			link_format.setAnchor(true);
			link_format.setAnchorHref("#" + event.name);
			cursor.insertText(event.name, link_format);
			cursor.insertText("(", monospace_format);
			std::vector<Arg>::const_iterator arg = event.args.cbegin();
			if (arg != event.args.cend()) {
				insertArg(*arg);
				arg++;
				while (arg != event.args.cend()) {
					cursor.insertText(", ", monospace_format);
					insertArg(*arg);
					arg++;
				}
			}
			cursor.insertText(")", monospace_format);
			cursor.insertBlock();
			insertDescription(event.description);
			cursor.insertBlock();
		};
		cursor.setBlockFormat(block_format);
		cursor.insertText(name, protocol_format);
		cursor.insertBlock();
		insertDescription(description);
		for (Interface const &interface: interfaces) {
			interface_format.setAnchor(true);
			interface_format.setAnchorNames({interface.name});
			interface_format.setAnchorHref("#" + interface.name);
			cursor.insertText(interface.name, interface_format);
			cursor.insertBlock();
			insertDescription(interface.description);
			cursor.insertBlock();
			for (Request const& request: interface.requests) {
				insertEvent(request);
			}
			for (Event const& event: interface.events) {
				insertEvent(event);
			}
			for (Enum const& e: interface.enums) {
				link_format.setAnchorNames({e.name});
				link_format.setAnchor(true);
				link_format.setAnchorHref("#" + e.name);
				cursor.insertText(e.name, link_format);
				cursor.insertBlock();
				QTextTable* table = cursor.insertTable(e.entries.size(), 3);
				for (size_t i = 0; i < e.entries.size(); i++) {
					Entry const& entry = e.entries[i];
					cursor.insertText(entry.name);
					cursor.movePosition(QTextCursor::NextCell);
					cursor.insertText(entry.value);
					cursor.movePosition(QTextCursor::NextCell);
					cursor.insertText(entry.summary);
					cursor.movePosition(QTextCursor::NextCell);
				}
				cursor.movePosition(QTextCursor::Down);
				insertDescription(e.description);
				cursor.insertBlock();
			}
		}
		// cursor.insertText("Copyright", summary_format);
		cursor.insertMarkdown(copyright);
	}
	void textDocument(QTextDocument *doc) const {
		QTextCursor cursor(doc);
		writeDocument(cursor);
	}
} Protocol;

typedef struct ProtocolDocument{
	std::filesystem::path path;
	Protocol protocol;
	QTextDocument *document = nullptr;
	ProtocolDocument(std::filesystem::path path): path(path), protocol(path), document(nullptr) {}

	QTextDocument* getDocument() {
		if (!document) {
			document = new QTextDocument;
			protocol.textDocument(document);
		}
		return this->document;
	}

	~ProtocolDocument() {
		if (document) delete document;
	}
} ProtocolDocument;

// Local Variables:
// c-file-style: "awk"
// compile-command: "ninja -C build && build/QtDocview"
// End:
