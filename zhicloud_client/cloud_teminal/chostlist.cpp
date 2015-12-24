#include "chostlist.h"
#include <QPushButton>
#include <QPainter>
CHostList::CHostList(QWidget *parent)
	: QDialog(parent)
{

	itemcount = 0;
	main_layout = new QVBoxLayout;
	//main_layout->setSpacing(20);
	main_layout->setAlignment(Qt::AlignCenter);
	main_layout->setContentsMargins(0, 0, 0, 10);
	curselectbtn = NULL;
	//setWindowOpacity(1);
	

}

CHostList::~CHostList()
{

}

void CHostList::AddCloudItem(QString name, K_yType& val)
{
	QPushButton* item = new QPushButton;
	item->setCheckable(true);
	item->setFixedSize(ITEM_W, ITEM_H);
	item->setText(name);
	item->setObjectName("itemBtn");
		item->setStyleSheet("QPushButton#itemBtn{border-image: url(:/first_login_res/input);}"
 		"QPushButton#itemBtn:hover{border-image: url(:/hostselect/itemhover);}"
 		"QPushButton#itemBtn:checked{background-image: url(:/hostselect/itemselected);}"
		);

	connect(item, SIGNAL(clicked()), this, SLOT(clickfunc()));
	itemcount++;
	main_layout->addWidget(item);

	if (!curselectbtn)
	{
		curselectbtn = item;
	}

	m_btnmap[item] = val;
}

void CHostList::endAdd()
{
	this->setFixedSize(270,(28+5) *itemcount - 5);
	this->setLayout(main_layout);

	
	curselectbtn->setChecked(true);

}

void CHostList::clickfunc()
{

	QPushButton* digitalbutton = (QPushButton *)sender();
	
	if (curselectbtn)
	{
		curselectbtn->setChecked(false);
	}

	digitalbutton->setChecked(true);
	curselectbtn = digitalbutton;

}

CHostList::K_yType CHostList::GetSelectItemInfo()
{
	return m_btnmap[curselectbtn];
}