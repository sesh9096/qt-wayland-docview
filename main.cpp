#include "protocol.hpp"
#include <filesystem>
#include <QApplication>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyleOption>
#include <QTextBrowser>
#include <QTextTable>
#include <QWidget>

class DocviewApplication : public QWidget {
public:
	DocviewApplication() {
		Protocol p("/usr/share/wayland/wayland.xml");

		// text_edit.setOpenExternalLinks(true);
		// text_edit.setReadOnly(true);
		QTextCursor cursor = text_edit.cursorForPosition({});
		// text_edit.document()->setBaseUrl(QUrl("application://wayland/"));
		p.writeDocument(cursor);

		text_edit.scrollToAnchor(p.name);
		text_edit.setWindowTitle(p.name);
		QObject::connect(&this->text_edit, &QTextBrowser::anchorClicked, this, &DocviewApplication::handleAnchorClicked);
	}
public slots:
	void handleAnchorClicked(const QUrl &link) {
		qDebug() << link;
	}
	void show() {
		text_edit.show();
	}
private:
	QTextBrowser text_edit;
	// QTextDocument document;
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
