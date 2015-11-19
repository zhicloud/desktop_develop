#ifndef CLEFTARRAWPWDEDIT_H
#define CLEFTARRAWPWDEDIT_H

#include <QWidget>
#include <QLineEdit>
class CMainWindow;
class CLeftArrawPWDEdit : public QWidget
{
	Q_OBJECT

public:
	CLeftArrawPWDEdit(QWidget *parent = 0);
	~CLeftArrawPWDEdit();
	void setMainWin(CMainWindow* main);
	QLineEdit* inputEdit;
public slots:
	void login();
private:
	CMainWindow* main_w;
};

#endif // CLEFTARRAWPWDEDIT_H
