/*
  Copyright (C) 2012-2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Kevin Krammer, kevin.krammer@kdab.com
  Author: Tobias Koenig, tobias.koenig@kdab.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "bookstore.h"

#include "booklistproxymodel.h"
#include "booksofauthormodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlTableModel>

BookStore::BookStore(QObject *parent)
  : QObject(parent)
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName("bookstore.db");
  if (!db.open()) {
    reportDbError( tr("Error When opening database"), db.lastError() );
    QCoreApplication::instance()->exit(-1);
  }

  m_authorTable = new QSqlTableModel(this);
  m_authorTable->setTable("author");
  m_authorTable->select();
  qDebug() << "authorTable.rowCount=" << m_authorTable->rowCount();
  m_authorTable->setHeaderData(1, Qt::Horizontal, tr("First Name"));
  m_authorTable->setHeaderData(2, Qt::Horizontal, tr("Last Name"));

  m_bookTable = new QSqlTableModel(this);
  m_bookTable->setTable("book");
  m_bookTable->select();
  m_bookTable->setHeaderData(1, Qt::Horizontal, tr("Title"));
  m_bookTable->setHeaderData(2, Qt::Horizontal, tr("Price"));
  m_bookTable->setHeaderData(4, Qt::Horizontal, tr("Notes"));

  m_booksOfAuthorTable = new BooksOfAuthorModel(this);

  m_bookList = new BookListProxyModel(this);

  QSqlQueryModel *bookListSourceModel = new QSqlQueryModel(this);
  m_bookList->setSourceModel(bookListSourceModel);
  bookListSourceModel->setQuery("SELECT book.id, book.title, book.price, book.notes, "
                                "author.id, author.firstname, author.surname "
                                "FROM book, author WHERE book.authorid = author.id");

  if (bookListSourceModel->lastError().type() != QSqlError::NoError)
    reportDbError(tr("Error running book list source query"), bookListSourceModel->lastError());

  m_bookList->addColumnToRoleMapping(0, BookListProxyModel::BookIdRole);
  m_bookList->addColumnToRoleMapping(1, BookListProxyModel::BookTitleRole);
  m_bookList->addColumnToRoleMapping(2, BookListProxyModel::BookPriceRole);
  m_bookList->addColumnToRoleMapping(3, BookListProxyModel::BookNotesRole);
  m_bookList->addColumnToRoleMapping(4, BookListProxyModel::AuthorIdRole);
  m_bookList->addColumnToRoleMapping(5, BookListProxyModel::AuthorFirstNameRole);
  m_bookList->addColumnToRoleMapping(6, BookListProxyModel::AuthorLastNameRole);
}

QObject *BookStore::authorTable() const
{
  return m_authorTable;
}

QObject *BookStore::bookTable() const
{
  return m_bookTable;
}

QObject *BookStore::booksOfAuthorTable() const
{
  return m_booksOfAuthorTable;
}

QObject *BookStore::bookList() const
{
  return m_bookList;
}

void BookStore::reportDbError(const QString &message, const QSqlError &error)
{
  const QString text = tr("%1\nDriver Message: %2\nDatabase Message %3")
                          .arg(message)
                          .arg(error.driverText())
                          .arg(error.databaseText());

  qCritical() << text;

  emit critical(text);
}

int BookStore::authorId(const QModelIndex &index) const
{
  if (index.model() != m_authorTable) {
    qWarning() << Q_FUNC_INFO << "index.model is not the authorModel";
    return -1;
  }

  const QModelIndex idIndex = m_authorTable->index(index.row(), 0, index.parent());
  return m_authorTable->data(idIndex).toInt();
}
