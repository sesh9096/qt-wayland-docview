#include "protocol.hpp"
#include <filesystem>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyleOption>
#include <QTextBrowser>
#include <QTextTable>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

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
public slots:
	void handleAnchorClicked(const QUrl &link) {
		QString path = link.path();
		if(path.length() != 0) text_edit.setDocument(protocols[path]->getDocument());
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
