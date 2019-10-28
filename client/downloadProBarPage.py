# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'downloadProcess.ui'
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

class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName(_fromUtf8("Dialog"))
        Dialog.resize(400, 279)
        self.label = QtGui.QLabel(Dialog)
        self.label.setGeometry(QtCore.QRect(40, 20, 87, 20))
        self.label.setObjectName(_fromUtf8("label"))
        self.layoutWidget = QtGui.QWidget(Dialog)
        self.layoutWidget.setGeometry(QtCore.QRect(40, 60, 321, 86))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.progressBar = QtGui.QProgressBar(self.layoutWidget)
        self.progressBar.setProperty("value", 0)
        self.progressBar.setObjectName(_fromUtf8("progressBar"))
        self.gridLayout.addWidget(self.progressBar, 0, 0, 1, 2)
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.sum_label = QtGui.QLabel(self.layoutWidget)
        self.sum_label.setObjectName(_fromUtf8("sum_label"))
        self.gridLayout.addWidget(self.sum_label, 1, 1, 1, 1)
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.gridLayout.addWidget(self.label_3, 2, 0, 1, 1)
        self.download_label = QtGui.QLabel(self.layoutWidget)
        self.download_label.setObjectName(_fromUtf8("download_label"))
        self.gridLayout.addWidget(self.download_label, 2, 1, 1, 1)
        self.begin_btn = QtGui.QPushButton(Dialog)
        self.begin_btn.setGeometry(QtCore.QRect(140, 190, 112, 36))
        self.begin_btn.setObjectName(_fromUtf8("begin_btn"))
        self.lineEdit = QtGui.QLineEdit(Dialog)
        self.lineEdit.setGeometry(QtCore.QRect(150, 150, 113, 34))
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))

        self.retranslateUi(Dialog)
        QtCore.QMetaObject.connectSlotsByName(Dialog)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(_translate("Dialog", "Dialog", None))
        self.label.setText(_translate("Dialog", "下载进度：", None))
        self.label_2.setText(_translate("Dialog", "总大小：", None))
        self.sum_label.setText(_translate("Dialog", "0", None))
        self.label_3.setText(_translate("Dialog", "已下载：", None))
        self.download_label.setText(_translate("Dialog", "0", None))
        self.begin_btn.setText(_translate("Dialog", "开始下载", None))

