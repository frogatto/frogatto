/********************************************************************************
** Form generated from reading ui file 'ui_mainwindow.ui'
**
** Created: Fri Nov 23 21:05:41 2007
**      by: Qt User Interface Compiler version 4.3.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_UI_MAINWINDOW_H
#define UI_UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QScrollBar>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>
#include "editorglwidget.hpp"

class Ui_MainWindow
{
public:
    QAction *action_Quit;
    QAction *action_Open;
    QAction *action_Save;
    QAction *actionZoom_In;
    QAction *actionZoom_Out;
    QAction *actionRotate_Left;
    QAction *actionRotate_Right;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionTilt_Up;
    QAction *actionTilt_Down;
    QAction *actionDerive_map;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout1;
    EditorGLWidget *editorGLWidget;
    QScrollBar *verticalScrollBar;
    QScrollBar *horizontalScrollBar;
    QMenuBar *menubar;
    QMenu *menuEdit;
    QMenu *menuView;
    QMenu *menu_File;
    QStatusBar *statusbar;
    QToolBar *tilesToolBar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainWindow)
    {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
    MainWindow->resize(1256, 1014);
    action_Quit = new QAction(MainWindow);
    action_Quit->setObjectName(QString::fromUtf8("action_Quit"));
    action_Open = new QAction(MainWindow);
    action_Open->setObjectName(QString::fromUtf8("action_Open"));
    action_Save = new QAction(MainWindow);
    action_Save->setObjectName(QString::fromUtf8("action_Save"));
    actionZoom_In = new QAction(MainWindow);
    actionZoom_In->setObjectName(QString::fromUtf8("actionZoom_In"));
    actionZoom_Out = new QAction(MainWindow);
    actionZoom_Out->setObjectName(QString::fromUtf8("actionZoom_Out"));
    actionRotate_Left = new QAction(MainWindow);
    actionRotate_Left->setObjectName(QString::fromUtf8("actionRotate_Left"));
    actionRotate_Right = new QAction(MainWindow);
    actionRotate_Right->setObjectName(QString::fromUtf8("actionRotate_Right"));
    actionUndo = new QAction(MainWindow);
    actionUndo->setObjectName(QString::fromUtf8("actionUndo"));
    actionRedo = new QAction(MainWindow);
    actionRedo->setObjectName(QString::fromUtf8("actionRedo"));
    actionTilt_Up = new QAction(MainWindow);
    actionTilt_Up->setObjectName(QString::fromUtf8("actionTilt_Up"));
    actionTilt_Down = new QAction(MainWindow);
    actionTilt_Down->setObjectName(QString::fromUtf8("actionTilt_Down"));
    actionDerive_map = new QAction(MainWindow);
    actionDerive_map->setObjectName(QString::fromUtf8("actionDerive_map"));
    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    gridLayout = new QGridLayout(centralwidget);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setHorizontalSpacing(6);
    gridLayout->setVerticalSpacing(6);
    gridLayout->setContentsMargins(9, 9, 9, 9);
    gridLayout1 = new QGridLayout();
    gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
    gridLayout1->setHorizontalSpacing(6);
    gridLayout1->setVerticalSpacing(6);
    gridLayout1->setContentsMargins(0, 0, 0, 0);
    editorGLWidget = new EditorGLWidget(centralwidget);
    editorGLWidget->setObjectName(QString::fromUtf8("editorGLWidget"));

    gridLayout1->addWidget(editorGLWidget, 0, 0, 1, 1);

    verticalScrollBar = new QScrollBar(centralwidget);
    verticalScrollBar->setObjectName(QString::fromUtf8("verticalScrollBar"));
    verticalScrollBar->setOrientation(Qt::Vertical);

    gridLayout1->addWidget(verticalScrollBar, 0, 1, 1, 1);

    horizontalScrollBar = new QScrollBar(centralwidget);
    horizontalScrollBar->setObjectName(QString::fromUtf8("horizontalScrollBar"));
    horizontalScrollBar->setOrientation(Qt::Horizontal);

    gridLayout1->addWidget(horizontalScrollBar, 1, 0, 1, 1);


    gridLayout->addLayout(gridLayout1, 0, 0, 1, 1);

    MainWindow->setCentralWidget(centralwidget);
    menubar = new QMenuBar(MainWindow);
    menubar->setObjectName(QString::fromUtf8("menubar"));
    menubar->setGeometry(QRect(0, 0, 1256, 25));
    menuEdit = new QMenu(menubar);
    menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
    menuView = new QMenu(menubar);
    menuView->setObjectName(QString::fromUtf8("menuView"));
    menu_File = new QMenu(menubar);
    menu_File->setObjectName(QString::fromUtf8("menu_File"));
    MainWindow->setMenuBar(menubar);
    statusbar = new QStatusBar(MainWindow);
    statusbar->setObjectName(QString::fromUtf8("statusbar"));
    MainWindow->setStatusBar(statusbar);
    tilesToolBar = new QToolBar(MainWindow);
    tilesToolBar->setObjectName(QString::fromUtf8("tilesToolBar"));
    tilesToolBar->setOrientation(Qt::Vertical);
    MainWindow->addToolBar(Qt::RightToolBarArea, tilesToolBar);
    toolBar = new QToolBar(MainWindow);
    toolBar->setObjectName(QString::fromUtf8("toolBar"));
    toolBar->setOrientation(Qt::Horizontal);
    MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

    menubar->addAction(menu_File->menuAction());
    menubar->addAction(menuEdit->menuAction());
    menubar->addAction(menuView->menuAction());
    menuEdit->addAction(actionUndo);
    menuEdit->addAction(actionRedo);
    menuView->addAction(actionZoom_In);
    menuView->addAction(actionZoom_Out);
    menu_File->addAction(action_Open);
    menu_File->addAction(action_Save);
    menu_File->addAction(actionDerive_map);
    menu_File->addAction(action_Quit);
    toolBar->addAction(action_Open);
    toolBar->addAction(action_Save);
    toolBar->addSeparator();

    retranslateUi(MainWindow);

    QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
    MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
    action_Quit->setText(QApplication::translate("MainWindow", "&Quit", 0, QApplication::UnicodeUTF8));
    action_Open->setText(QApplication::translate("MainWindow", "&Open", 0, QApplication::UnicodeUTF8));
    action_Save->setText(QApplication::translate("MainWindow", "&Save", 0, QApplication::UnicodeUTF8));
    actionZoom_In->setText(QApplication::translate("MainWindow", "Zoom In", 0, QApplication::UnicodeUTF8));
    actionZoom_In->setShortcut(QString());
    actionZoom_Out->setText(QApplication::translate("MainWindow", "Zoom Out", 0, QApplication::UnicodeUTF8));
    actionZoom_Out->setShortcut(QString());
    actionRotate_Left->setText(QApplication::translate("MainWindow", "Rotate Left", 0, QApplication::UnicodeUTF8));
    actionRotate_Left->setShortcut(QString());
    actionRotate_Right->setText(QApplication::translate("MainWindow", "Rotate Right", 0, QApplication::UnicodeUTF8));
    actionRotate_Right->setShortcut(QString());
    actionUndo->setText(QApplication::translate("MainWindow", "Undo", 0, QApplication::UnicodeUTF8));
    actionRedo->setText(QApplication::translate("MainWindow", "Redo", 0, QApplication::UnicodeUTF8));
    actionTilt_Up->setText(QApplication::translate("MainWindow", "Tilt Up", 0, QApplication::UnicodeUTF8));
    actionTilt_Up->setShortcut(QString());
    actionTilt_Down->setText(QApplication::translate("MainWindow", "Tilt Down", 0, QApplication::UnicodeUTF8));
    actionTilt_Down->setShortcut(QString());
    actionDerive_map->setText(QApplication::translate("MainWindow", "Derive map...", 0, QApplication::UnicodeUTF8));
    menuEdit->setTitle(QApplication::translate("MainWindow", "Edit", 0, QApplication::UnicodeUTF8));
    menuView->setTitle(QApplication::translate("MainWindow", "View", 0, QApplication::UnicodeUTF8));
    menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0, QApplication::UnicodeUTF8));
    toolBar->setWindowTitle(QApplication::translate("MainWindow", "toolBar", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

#endif // UI_UI_MAINWINDOW_H
