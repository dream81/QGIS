/***************************************************************************
  qgsvectortilebasicrendererwidget.cpp
  --------------------------------------
  Date                 : April 2020
  Copyright            : (C) 2020 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvectortilebasicrendererwidget.h"

#include "qgsguiutils.h"
#include "qgssymbollayerutils.h"
#include "qgsvectortilebasicrenderer.h"
#include "qgsvectortilelayer.h"
#include "qgssymbolselectordialog.h"
#include "qgsstyle.h"

#include <QAbstractListModel>
#include <QInputDialog>
#include <QMenu>


///@cond PRIVATE

class QgsVectorTileBasicRendererListModel : public QAbstractListModel
{
  public:
    QgsVectorTileBasicRendererListModel( QgsVectorTileBasicRenderer *r, QObject *parent = nullptr );

    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    Qt::ItemFlags flags( const QModelIndex &index ) const override;
    bool setData( const QModelIndex &index, const QVariant &value, int role ) override;

    bool removeRows( int row, int count, const QModelIndex &parent = QModelIndex() ) override;

    void insertStyle( int row, const QgsVectorTileBasicRendererStyle &style );

    // drag'n'drop support
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData( const QModelIndexList &indexes ) const override;
    bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent ) override;

  private:
    QgsVectorTileBasicRenderer *mRenderer = nullptr;
};


QgsVectorTileBasicRendererListModel::QgsVectorTileBasicRendererListModel( QgsVectorTileBasicRenderer *r, QObject *parent )
  : QAbstractListModel( parent )
  , mRenderer( r )
{
}

int QgsVectorTileBasicRendererListModel::rowCount( const QModelIndex &parent ) const
{
  if ( parent.isValid() )
    return 0;

  return mRenderer->styles().count();
}

int QgsVectorTileBasicRendererListModel::columnCount( const QModelIndex & ) const
{
  return 5;
}

QVariant QgsVectorTileBasicRendererListModel::data( const QModelIndex &index, int role ) const
{
  if ( index.row() < 0 || index.row() >= mRenderer->styles().count() )
    return QVariant();

  const QList<QgsVectorTileBasicRendererStyle> styles = mRenderer->styles();
  const QgsVectorTileBasicRendererStyle &style = styles[index.row()];

  switch ( role )
  {
    case Qt::DisplayRole:
    {
      if ( index.column() == 0 )
        return style.styleName();
      else if ( index.column() == 1 )
        return style.layerName().isEmpty() ? tr( "(all layers)" ) : style.layerName();
      else if ( index.column() == 2 )
        return style.minZoomLevel() >= 0 ? style.minZoomLevel() : QVariant();
      else if ( index.column() == 3 )
        return style.maxZoomLevel() >= 0 ? style.maxZoomLevel() : QVariant();
      else if ( index.column() == 4 )
        return style.filterExpression().isEmpty() ? tr( "(no filter)" ) : style.filterExpression();

      break;
    }

    case Qt::EditRole:
    {
      if ( index.column() == 0 )
        return style.styleName();
      else if ( index.column() == 1 )
        return style.layerName();
      else if ( index.column() == 2 )
        return style.minZoomLevel();
      else if ( index.column() == 3 )
        return style.maxZoomLevel();
      else if ( index.column() == 4 )
        return style.filterExpression();

      break;
    }

    case Qt::DecorationRole:
    {
      if ( index.column() == 0 && style.symbol() )
      {
        const int iconSize = QgsGuiUtils::scaleIconSize( 16 );
        return QgsSymbolLayerUtils::symbolPreviewIcon( style.symbol(), QSize( iconSize, iconSize ) );
      }
      break;
    }

    case Qt::CheckStateRole:
    {
      if ( index.column() != 0 )
        return QVariant();
      return style.isEnabled() ? Qt::Checked : Qt::Unchecked;
    }

  }
  return QVariant();
}

QVariant QgsVectorTileBasicRendererListModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 && section < 5 )
  {
    QStringList lst;
    lst << tr( "Label" ) << tr( "Layer" ) << tr( "Min. zoom" ) << tr( "Max. zoom" ) << tr( "Filter" );
    return lst[section];
  }

  return QVariant();
}

Qt::ItemFlags QgsVectorTileBasicRendererListModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return Qt::ItemIsDropEnabled;

  Qt::ItemFlag checkable = ( index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags );

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
         Qt::ItemIsEditable | checkable |
         Qt::ItemIsDragEnabled;
}

bool QgsVectorTileBasicRendererListModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( !index.isValid() )
    return false;

  QgsVectorTileBasicRendererStyle style = mRenderer->style( index.row() );

  if ( role == Qt::CheckStateRole )
  {
    style.setEnabled( value.toInt() == Qt::Checked );
    mRenderer->setStyle( index.row(), style );
    emit dataChanged( index, index );
    return true;
  }

  if ( role == Qt::EditRole )
  {
    if ( index.column() == 0 )
      style.setStyleName( value.toString() );
    else if ( index.column() == 1 )
      style.setLayerName( value.toString() );
    else if ( index.column() == 2 )
      style.setMinZoomLevel( value.toInt() );
    else if ( index.column() == 3 )
      style.setMaxZoomLevel( value.toInt() );
    else if ( index.column() == 4 )
      style.setFilterExpression( value.toString() );

    mRenderer->setStyle( index.row(), style );
    emit dataChanged( index, index );
    return true;
  }

  return false;
}

bool QgsVectorTileBasicRendererListModel::removeRows( int row, int count, const QModelIndex &parent )
{
  QList<QgsVectorTileBasicRendererStyle> styles = mRenderer->styles();

  if ( row < 0 || row >= styles.count() )
    return false;

  beginRemoveRows( parent, row, row + count - 1 );

  for ( int i = 0; i < count; i++ )
  {
    if ( row < styles.count() )
    {
      styles.removeAt( row );
    }
  }

  mRenderer->setStyles( styles );

  endRemoveRows();
  return true;
}

void QgsVectorTileBasicRendererListModel::insertStyle( int row, const QgsVectorTileBasicRendererStyle &style )
{
  beginInsertRows( QModelIndex(), row, row );

  QList<QgsVectorTileBasicRendererStyle> styles = mRenderer->styles();
  styles.insert( row, style );
  mRenderer->setStyles( styles );

  endInsertRows();
}

Qt::DropActions QgsVectorTileBasicRendererListModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

QStringList QgsVectorTileBasicRendererListModel::mimeTypes() const
{
  QStringList types;
  types << QStringLiteral( "application/vnd.text.list" );
  return types;
}

QMimeData *QgsVectorTileBasicRendererListModel::mimeData( const QModelIndexList &indexes ) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream( &encodedData, QIODevice::WriteOnly );

  const auto constIndexes = indexes;
  for ( const QModelIndex &index : constIndexes )
  {
    // each item consists of several columns - let's add it with just first one
    if ( !index.isValid() || index.column() != 0 )
      continue;

    QgsVectorTileBasicRendererStyle style = mRenderer->style( index.row() );

    QDomDocument doc;
    QDomElement rootElem = doc.createElement( QStringLiteral( "vector_tile_basic_renderer_style_mime" ) );
    style.writeXml( rootElem, QgsReadWriteContext() );
    doc.appendChild( rootElem );

    stream << doc.toString( -1 );
  }

  mimeData->setData( QStringLiteral( "application/vnd.text.list" ), encodedData );
  return mimeData;
}

bool QgsVectorTileBasicRendererListModel::dropMimeData( const QMimeData *data,
    Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
  Q_UNUSED( column )

  if ( action == Qt::IgnoreAction )
    return true;

  if ( !data->hasFormat( QStringLiteral( "application/vnd.text.list" ) ) )
    return false;

  if ( parent.column() > 0 )
    return false;

  QByteArray encodedData = data->data( QStringLiteral( "application/vnd.text.list" ) );
  QDataStream stream( &encodedData, QIODevice::ReadOnly );
  int rows = 0;

  if ( row == -1 )
  {
    // the item was dropped at a parent - we may decide where to put the items - let's append them
    row = rowCount( parent );
  }

  while ( !stream.atEnd() )
  {
    QString text;
    stream >> text;

    QDomDocument doc;
    if ( !doc.setContent( text ) )
      continue;
    QDomElement rootElem = doc.documentElement();
    if ( rootElem.tagName() != QLatin1String( "vector_tile_basic_renderer_style_mime" ) )
      continue;

    QgsVectorTileBasicRendererStyle style;
    style.readXml( rootElem, QgsReadWriteContext() );

    insertStyle( row + rows, style );
    ++rows;
  }
  return true;
}


//


QgsVectorTileBasicRendererWidget::QgsVectorTileBasicRendererWidget( QgsVectorTileLayer *layer, QgsMapCanvas *canvas, QgsMessageBar *messageBar, QWidget *parent )
  : QgsMapLayerConfigWidget( layer, canvas, parent )
  , mVTLayer( layer )
  , mMessageBar( messageBar )
{
  setupUi( this );
  layout()->setContentsMargins( 0, 0, 0, 0 );

  if ( layer->renderer() && layer->renderer()->type() == QStringLiteral( "basic" ) )
  {
    mRenderer.reset( static_cast<QgsVectorTileBasicRenderer *>( layer->renderer()->clone() ) );
  }
  else
  {
    mRenderer.reset( new QgsVectorTileBasicRenderer() );
  }

  mModel = new QgsVectorTileBasicRendererListModel( mRenderer.get(), viewStyles );
  viewStyles->setModel( mModel );

  QMenu *menuAddRule = new QMenu( btnAddRule );
  menuAddRule->addAction( tr( "Marker" ), this, [this] { addStyle( QgsWkbTypes::PointGeometry ); } );
  menuAddRule->addAction( tr( "Line" ), this, [this] { addStyle( QgsWkbTypes::LineGeometry ); } );
  menuAddRule->addAction( tr( "Fill" ), this, [this] { addStyle( QgsWkbTypes::PolygonGeometry ); } );
  btnAddRule->setMenu( menuAddRule );

  connect( btnEditRule, &QPushButton::clicked, this, &QgsVectorTileBasicRendererWidget::editStyle );
  connect( btnRemoveRule, &QAbstractButton::clicked, this, &QgsVectorTileBasicRendererWidget::removeStyle );

  connect( viewStyles, &QAbstractItemView::doubleClicked, this, &QgsVectorTileBasicRendererWidget::editStyleAtIndex );

  connect( mModel, &QAbstractItemModel::dataChanged, this, &QgsPanelWidget::widgetChanged );
  connect( mModel, &QAbstractItemModel::rowsInserted, this, &QgsPanelWidget::widgetChanged );
  connect( mModel, &QAbstractItemModel::rowsRemoved, this, &QgsPanelWidget::widgetChanged );
}

QgsVectorTileBasicRendererWidget::~QgsVectorTileBasicRendererWidget() = default;

void QgsVectorTileBasicRendererWidget::apply()
{
  mVTLayer->setRenderer( mRenderer->clone() );
}

void QgsVectorTileBasicRendererWidget::addStyle( QgsWkbTypes::GeometryType geomType )
{
  QgsVectorTileBasicRendererStyle style( QString(), QString(), geomType );
  style.setSymbol( QgsSymbol::defaultSymbol( geomType ) );

  int rows = mModel->rowCount();
  mModel->insertStyle( rows, style );
  viewStyles->selectionModel()->setCurrentIndex( mModel->index( rows, 0 ), QItemSelectionModel::ClearAndSelect );
}

void QgsVectorTileBasicRendererWidget::editStyle()
{
  editStyleAtIndex( viewStyles->selectionModel()->currentIndex() );
}

void QgsVectorTileBasicRendererWidget::editStyleAtIndex( const QModelIndex &index )
{
  QgsVectorTileBasicRendererStyle style = mRenderer->style( index.row() );

  if ( !style.symbol() )
    return;

  std::unique_ptr< QgsSymbol > symbol( style.symbol()->clone() );

  QgsSymbolWidgetContext context;
  context.setMapCanvas( mMapCanvas );
  context.setMessageBar( mMessageBar );

  QgsVectorLayer *vectorLayer = nullptr;  // TODO: have a temporary vector layer with sub-layer's fields?

  QgsPanelWidget *panel = QgsPanelWidget::findParentPanel( this );
  if ( panel && panel->dockMode() )
  {
    QgsSymbolSelectorWidget *dlg = new QgsSymbolSelectorWidget( symbol.release(), QgsStyle::defaultStyle(), vectorLayer, panel );
    dlg->setContext( context );
    dlg->setPanelTitle( style.styleName() );
    connect( dlg, &QgsPanelWidget::widgetChanged, this, &QgsVectorTileBasicRendererWidget::updateSymbolsFromWidget );
    connect( dlg, &QgsPanelWidget::panelAccepted, this, &QgsVectorTileBasicRendererWidget::cleanUpSymbolSelector );
    openPanel( dlg );
  }
  else
  {
    QgsSymbolSelectorDialog dlg( symbol.get(), QgsStyle::defaultStyle(), vectorLayer, panel );
    dlg.setContext( context );
    if ( !dlg.exec() || !symbol )
    {
      return;
    }

    style.setSymbol( symbol.release() );
    mRenderer->setStyle( index.row(), style );
    emit widgetChanged();
  }
}

void QgsVectorTileBasicRendererWidget::updateSymbolsFromWidget()
{
  int index = viewStyles->selectionModel()->currentIndex().row();
  QgsVectorTileBasicRendererStyle style = mRenderer->style( index );

  QgsSymbolSelectorWidget *dlg = qobject_cast<QgsSymbolSelectorWidget *>( sender() );
  style.setSymbol( dlg->symbol()->clone() );

  mRenderer->setStyle( index, style );
  emit widgetChanged();
}

void QgsVectorTileBasicRendererWidget::cleanUpSymbolSelector( QgsPanelWidget *container )
{
  QgsSymbolSelectorWidget *dlg = qobject_cast<QgsSymbolSelectorWidget *>( container );
  if ( !dlg )
    return;

  delete dlg->symbol();
}

void QgsVectorTileBasicRendererWidget::removeStyle()
{
  QItemSelection sel = viewStyles->selectionModel()->selection();
  const auto constSel = sel;
  for ( const QItemSelectionRange &range : constSel )
  {
    if ( range.isValid() )
      mModel->removeRows( range.top(), range.bottom() - range.top() + 1, range.parent() );
  }
  // make sure that the selection is gone
  viewStyles->selectionModel()->clear();
}

///@endcond
