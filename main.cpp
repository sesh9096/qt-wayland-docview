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
	DocviewApplication() : wayland("/usr/share/wayland/wayland.xml") {
		// std::filesystem::path protocols_dir = "/usr/share/wayland-protocols";
		for(auto const& type: std::filesystem::directory_iterator{"/usr/share/wayland-protocols"})
			for(auto const& subdir: std::filesystem::directory_iterator{type})
				for(auto const& file: std::filesystem::directory_iterator{subdir.path()})
					protocols[type.path().filename()].emplace_back(file.path());

		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious), "Previous");
		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoNext), "Next");
		toolbar.addAction(QIcon::fromTheme(QIcon::ThemeIcon::GoHome), "Home");
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setContentsMargins({});
		layout->addWidget(&toolbar);
		layout->addWidget(&text_edit);

		createIndex();
		text_edit.setOpenLinks(false);
		text_edit.setDocument(&index);
		text_edit.setWindowTitle("Wayland Docview");
		QObject::connect(&this->text_edit, &QTextBrowser::anchorClicked, this, &DocviewApplication::handleAnchorClicked);
		QObject::connect(&this->toolbar, &QToolBar::actionTriggered, this, &DocviewApplication::barActionTriggered);
	}
	void createIndex() {
		index.clear();
		QTextCursor cursor(&index);
		QTextCharFormat heading_format;
		heading_format.setFontPointSize(heading_format.font().pointSize() + 2);
		heading_format.setFontWeight(QFont::Bold);
		QTextCharFormat link_format;
		link_format.setAnchor(true);
		link_format.setFontUnderline(true);

		cursor.insertText("core", heading_format);
		link_format.setAnchorHref("wayland");
		cursor.insertBlock();
		cursor.insertText("wayland", link_format);
		cursor.insertBlock();

		for(auto const &pair: protocols) {
			cursor.insertText(QString::fromStdString(pair.first), heading_format);
			cursor.insertBlock();
			for(auto const &protocol: pair.second) {
				link_format.setAnchorHref(protocol.protocol.name);
				cursor.insertText(protocol.protocol.name, link_format);
				cursor.insertBlock();
			}
		}
	}
public slots:
	void handleAnchorClicked(const QUrl &link) {
		for(auto &pair: protocols) {
			for(auto &protocol: pair.second) {
				if(link == protocol.protocol.name) {
					text_edit.setDocument(protocol.getDocument());
					return;
				}
			}
		}
		text_edit.scrollToAnchor(link.fragment());
		qDebug() << link;
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
private:
	QTextBrowser text_edit;
	QToolBar toolbar;
	// owned by toolbar
	QTextDocument index;
	ProtocolDocument wayland;
	std::unordered_map<std::string, std::vector<ProtocolDocument>> protocols;
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
