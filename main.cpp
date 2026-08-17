#include "protocol.hpp"
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyleOption>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextTable>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <filesystem>

typedef struct ProtocolDocument{
	std::filesystem::path path;
	Protocol protocol;
	QTextDocument *document = nullptr;
	ProtocolDocument(std::filesystem::path path): path(path), protocol(path), document(nullptr) {}

	~ProtocolDocument() {
		if (document) delete document;
	}
} ProtocolDocument;

class DocviewApplication : public QWidget {
public:
	DocviewApplication() {
		addProtocol("/usr/share/wayland/wayland.xml", "core");
		for(auto const& type: std::filesystem::directory_iterator{"/usr/share/wayland-protocols"})
			for(auto const& subdir: std::filesystem::directory_iterator{type})
				for(auto const& file: std::filesystem::directory_iterator{subdir.path()})
					addProtocol(file.path(), type.path().filename());

		line_edit = new QLineEdit;
		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious), "Previous");
		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoNext), "Next");
		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoHome), "Home");
		toolbar.addWidget(line_edit);
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setContentsMargins({});
		layout->addWidget(&toolbar);
		layout->addWidget(&text_edit);

		createIndex();
		text_edit.setOpenLinks(false);
		text_edit.setDocument(&index);
		text_edit.setWindowTitle("Wayland Docview");
		// text_edit.setCornerWidget();
		// QObject::connect(line_edit, &QLineEdit::textChanged, this, &DocviewApplication::searchTextChanged);
		QObject::connect(line_edit, &QLineEdit::returnPressed, this, &DocviewApplication::searchReturnPressed);
		QObject::connect(&this->text_edit, &QTextBrowser::anchorClicked, this, &DocviewApplication::handleAnchorClicked);
		QObject::connect(&this->toolbar, &QToolBar::actionTriggered, this, &DocviewApplication::barActionTriggered);
	}
	~DocviewApplication() {
		for(auto pair: protocols) {
			delete pair.second;
		}
	}
	void createIndex() {
		index.clear();
		// default 40
		index.setIndentWidth(10);
		QTextCursor cursor(&index);
		QTextCharFormat heading_format;
		heading_format.setFontPointSize(heading_format.font().pointSize() + 2);
		heading_format.setFontWeight(QFont::Bold);
		QTextCharFormat link_format;
		link_format.setAnchor(true);
		link_format.setFontUnderline(true);
		QTextListFormat list_format;
		list_format.setStyle(QTextListFormat::ListDisc);

		for(auto const &phase: phases) {
			if(phase_lookup.count(phase) == 0) continue;
			cursor.insertText(phase, heading_format);
			QTextBlockFormat block_format = cursor.blockFormat();
			cursor.setCharFormat(link_format);
			cursor.insertList(list_format);
			for(auto const &pd: phase_lookup[phase]) {
				link_format.setAnchorHref(pd->protocol.name);
				cursor.insertText(pd->protocol.name, link_format);
				cursor.insertBlock();
			}
			cursor.setBlockFormat(block_format);
		}
	}
	void addProtocol(std::filesystem::path const& path, std::string const& phase = "custom") {
		ProtocolDocument *pd = new ProtocolDocument(path);
		protocols[pd->protocol.name] = pd;
		phase_lookup[phase].push_back(pd);
		for(auto const& interface: pd->protocol.interfaces) interface_lookup[interface.name] = pd;
	}

	QTextDocument* getDocument(ProtocolDocument *pd) {
		if (!pd->document) createDocument(pd);
		return pd->document;
	}
	void createDocument(ProtocolDocument *pd) {
		pd->document = new QTextDocument;
		QTextCursor cursor(pd->document);
		Protocol &protocol = pd->protocol;
		std::vector<Interface>const & interfaces = protocol.interfaces;

		QTextBlockFormat block_format;
		block_format.setBottomMargin(5);
		QTextCharFormat protocol_format;
		protocol_format.setFontPointSize(protocol_format.font().pointSize() + 5);
		protocol_format.setFontWeight(QFont::Bold);
		protocol_format.setAnchorNames({protocol.name});
		protocol_format.setAnchorHref("#" + protocol.name);
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
			if (arg.allow_null) cursor.insertText("?");
			cursor.insertText(arg.type);
			if (arg.interface.length() != 0) {
				cursor.insertText("<");
				link_format.setAnchor(true);
				if (std::find_if(interfaces.begin(), interfaces.end(), [=] (Interface const& i) {return i.name == arg.interface;}) == interfaces.end()) {
					// find in other protocols
					link_format.setAnchorHref(interface_lookup[arg.interface]->protocol.name + "#" + arg.interface);
				} else {
					link_format.setAnchorHref("#" + arg.interface);
				}
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
		cursor.insertText(protocol.name, protocol_format);
		cursor.insertBlock();
		insertDescription(protocol.description);
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
		cursor.insertMarkdown(protocol.copyright);
	}
public slots:
	void handleAnchorClicked(const QUrl &link) {
		QString path = link.path();
		if(path.length() != 0) text_edit.setDocument(getDocument(protocols[path]));
		text_edit.scrollToAnchor(link.fragment());
	}
	void barActionTriggered(QAction *action) {
		if(action->iconText() == "Home") {
			text_edit.setDocument(&index);
		} else if(action->iconText() == "Previous") {
			text_edit.backward();
		} else if(action->iconText() == "Next") {
			text_edit.forward();
		}
	}
	// void searchTextChanged(const QString &text) {
	// 	text_edit.find(text);
	// 	// qDebug() << text;
	// }
	void searchReturnPressed() {
		QString substr = line_edit->text();
		if(!text_edit.find(substr)) {
			QTextDocument *document = text_edit.document();
			QTextCursor cur = document->find(substr);
			if(!cur.isNull()) {
				text_edit.moveCursor(QTextCursor::Start);
				text_edit.find(substr);
			}
		}
		// qDebug() << text;
	}
private:
	QTextBrowser text_edit;
	QToolBar toolbar;
	// owned by toolbar
	QLineEdit *line_edit = nullptr;
	QTextDocument index;
	std::unordered_map<QString, ProtocolDocument*> protocols;
	std::unordered_map<std::string, std::vector<ProtocolDocument*>> phase_lookup;
	std::unordered_map<QString, ProtocolDocument*> interface_lookup;
	const char* phases[5] = {"core", "stable", "unstable", "staging", "experimental"};
// signals:
};

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	DocviewApplication docview;
	docview.show();
	return app.exec();
}

// Local Variables:
// c-file-style: "awk"
// compile-command: "ninja -C build && build/QtDocview"
// End:
