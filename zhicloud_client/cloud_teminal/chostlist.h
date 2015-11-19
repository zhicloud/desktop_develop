#ifndef CHOSTLIST_H
#define CHOSTLIST_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDialog>
#include <map>
using namespace std;
#define ITEM_W 270
#define ITEM_H 28
class CHostList : public QDialog
{
	Q_OBJECT

public:
	CHostList(QWidget *parent = 0);
	~CHostList();
	typedef map<QString, QString> K_yType;
	void AddCloudItem(QString name, K_yType& val);
	void endAdd();

	K_yType GetSelectItemInfo();

public slots:
	void clickfunc();

private:
	int itemcount;
	QVBoxLayout* main_layout;
	QPushButton* curselectbtn;

	map<QPushButton*, K_yType>m_btnmap;

};

#endif // CHOSTLIST_H
