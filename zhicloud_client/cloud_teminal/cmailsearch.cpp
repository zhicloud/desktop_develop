#include "cmailsearch.h"
#include <QPushButton>
CMailSearch::CMailSearch(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(169, 24);
	this->setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/mailbox/msearch")));
	this->setPalette(palette);
	QLineEdit* inputEdit = new QLineEdit();
	inputEdit->setFixedSize(100, 22);
	inputEdit->setPlaceholderText(QStringLiteral("邮件搜索"));
	inputEdit->setStyleSheet("background-color:transparent;color:black;font-size:11px;border:0px;");

	QPushButton* search = new QPushButton;
	search->setFixedSize(12, 12);
	search->setObjectName("search");
	search->setStyleSheet("QPushButton#search{border-image: url(:/mailbox/search);}"
		"QPushButton#search:hover{border-image: url(:/mailbox/search);}"
		"QPushButton#search:pressed{border-image: url(:/mailbox/searchclick);}");

	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->setAlignment(Qt::AlignCenter);
	hLayout->setContentsMargins(5, 0, 5, 0);
	hLayout->addWidget(inputEdit);
	hLayout->addWidget(search);
	this->setLayout(hLayout);
}

CMailSearch::~CMailSearch()
{

}
