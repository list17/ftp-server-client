# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'main.ui'
#
# Created by: PyQt4 UI code generator 4.12.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_ListFTP(object):
    def setupUi(self, ListFTP):
        ListFTP.setObjectName(_fromUtf8("ListFTP"))
        ListFTP.resize(800, 600)
        self.centralwidget = QtGui.QWidget(ListFTP)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.listWidget = QtGui.QListWidget(self.centralwidget)
        self.listWidget.setGeometry(QtCore.QRect(0, 20, 801, 481))
        self.listWidget.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.listWidget.setObjectName(_fromUtf8("listWidget"))
        ListFTP.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(ListFTP)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 800, 32))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        ListFTP.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(ListFTP)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        ListFTP.setStatusBar(self.statusbar)

        self.retranslateUi(ListFTP)
        QtCore.QMetaObject.connectSlotsByName(ListFTP)

    def retranslateUi(self, ListFTP):
        ListFTP.setWindowTitle(_translate("ListFTP", "MainWindow", None))

