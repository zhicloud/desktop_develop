#ifndef CLOUD_TERMINAL_H
#define CLOUD_TERMINAL_H

#include <QtWidgets/QMainWindow>
#include "ui_cloud_terminal.h"

class cloud_terminal : public QMainWindow
{
	Q_OBJECT

public:
	cloud_terminal(QWidget *parent = 0);
	~cloud_terminal();

private:
	Ui::cloud_terminalClass ui;
};

#endif // CLOUD_TERMINAL_H
