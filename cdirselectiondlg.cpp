#include "cdirselectiondlg.h"
#include "scriptingconsole.h"

#include <QLabel>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLineEdit>
#include <qdebug.h>

CDirSelectionDlg::CDirSelectionDlg(const QString initialPath, QWidget *parent) : QDialog(parent), m_initialPath(initialPath)
{
    setMinimumSize(200, 300);
    resize(600, 430);
    m_model = new QFileSystemModel(this);
    auto rootIdx = m_model->setRootPath(m_initialPath);
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setHeaderHidden(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    for(int i = 1; i < m_model->columnCount(); i ++)    // don't show Size, Type, etc.
        m_treeView->setColumnHidden(i, true);
    m_treeView->scrollTo(rootIdx);
    m_treeView->selectionModel()->setCurrentIndex(rootIdx, QItemSelectionModel::Current | QItemSelectionModel::Select);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CDirSelectionDlg::onCurrentChanged);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CDirSelectionDlg::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CDirSelectionDlg::reject);
    m_OKbutton = buttonBox->button(QDialogButtonBox::Ok);
    m_Cancelbutton = buttonBox->button(QDialogButtonBox::Cancel);

    auto label = new QLabel(tr("Folder:"));
    m_folderName = new QLineEdit(this);
    m_folderName->setReadOnly(false);
    m_folderName->setText(QFileInfo(m_initialPath).fileName());
    auto pathLayout = new QHBoxLayout();
    pathLayout->addWidget(label);
    pathLayout->addSpacing(10);
    pathLayout->addWidget(m_folderName);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_treeView);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(pathLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

void CDirSelectionDlg::accept(void)
{
    this->setResult(1);
    this->hide();
}

void CDirSelectionDlg::reject(void)
{
    this->setResult(0);
    this->hide();
}

void CDirSelectionDlg::setTitle(QString title)
{
    this->setWindowTitle(title);
}

void CDirSelectionDlg::onCurrentChanged()
{
    auto fileInfo = m_model->fileInfo(m_treeView->selectionModel()->currentIndex());
    m_folderName->setText(fileInfo.fileName());
    m_OKbutton->setEnabled(fileInfo.isDir());
    m_OKbutton->setDefault(fileInfo.isDir());
}

QString CDirSelectionDlg::selectedPath(void)
{
    auto fileInfo = m_model->fileInfo(m_treeView->selectionModel()->currentIndex());
    return fileInfo.canonicalPath() + "/" + m_folderName->text();
}

QDir CDirSelectionDlg::directory() const
{
    return QDir(m_model->fileInfo(m_treeView->selectionModel()->currentIndex()).absoluteFilePath());
}
