/***************************************************************************
                         qgscomposerattributetable.cpp
                         -----------------------------
    begin                : April 2010
    copyright            : (C) 2010 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerattributetable.h"
#include "qgscomposermap.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectorlayer.h"

QgsComposerAttributeTableCompare::QgsComposerAttributeTableCompare()
    : mCurrentSortColumn( 0 ), mAscending( true )
{
}


bool QgsComposerAttributeTableCompare::operator()( const QgsAttributeMap& m1, const QgsAttributeMap& m2 )
{
  QVariant v1 = m1[mCurrentSortColumn];
  QVariant v2 = m2[mCurrentSortColumn];

  bool less = false;
  if ( v1.type() == QVariant::String && v2.type() == QVariant::String )
  {
    less = v1.toString() < v2.toString();
  }
  else
  {
    less = v1.toDouble() < v2.toDouble();
  }
  return ( mAscending ? less : !less );
}


QgsComposerAttributeTable::QgsComposerAttributeTable( QgsComposition* composition )
    : QgsComposerTable( composition )
    , mVectorLayer( 0 )
    , mComposerMap( 0 )
    , mMaximumNumberOfFeatures( 5 )
    , mShowOnlyVisibleFeatures( true )
    , mFilterFeatures( false )
    , mFeatureFilter( "" )
{
  //set first vector layer from layer registry as default one
  QMap<QString, QgsMapLayer*> layerMap =  QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::const_iterator mapIt = layerMap.constBegin();
  for ( ; mapIt != layerMap.constEnd(); ++mapIt )
  {
    QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>( mapIt.value() );
    if ( vl )
    {
      mVectorLayer = vl;
      break;
    }
  }
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( removeLayer( const QString& ) ) );
}

QgsComposerAttributeTable::~QgsComposerAttributeTable()
{
}

void QgsComposerAttributeTable::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  if ( mComposerMap && mComposerMap->isDrawing() )
  {
    return;
  }
  QgsComposerTable::paint( painter, itemStyle, pWidget );
}

void QgsComposerAttributeTable::initializeAliasMap()
{
  mFieldAliasMap.clear();
  if ( mVectorLayer )
  {
    const QgsFields& fields = mVectorLayer->pendingFields();
    for ( int idx = 0; idx < fields.count(); ++idx )
    {
      QString currentAlias = mVectorLayer->attributeAlias( idx );
      if ( !currentAlias.isEmpty() )
      {
        mFieldAliasMap.insert( idx, currentAlias );
      }
    }
  }
}

void QgsComposerAttributeTable::setVectorLayer( QgsVectorLayer* layer )
{
  if ( layer == mVectorLayer )
  {
    return;
  }

  mDisplayAttributes.clear();

  if ( mVectorLayer )
  {
    //disconnect from previous layer
    QObject::disconnect( mVectorLayer, SIGNAL( layerModified() ), this, SLOT( refreshAttributes() ) );
  }

  mVectorLayer = layer;
  initializeAliasMap();
  refreshAttributes();

  //listen for modifications to layer and refresh table when they occur
  QObject::connect( mVectorLayer, SIGNAL( layerModified() ), this, SLOT( refreshAttributes() ) );
}

void QgsComposerAttributeTable::setComposerMap( const QgsComposerMap* map )
{
  if ( map == mComposerMap )
  {
    return;
  }

  if ( mComposerMap )
  {
    //disconnect from previous map
    QObject::disconnect( mComposerMap, SIGNAL( extentChanged() ), this, SLOT( refreshAttributes() ) );
  }
  mComposerMap = map;
  if ( mComposerMap )
  {
    //listen out for extent changes in linked map
    QObject::connect( mComposerMap, SIGNAL( extentChanged() ), this, SLOT( refreshAttributes() ) );
  }
  refreshAttributes();
}

void QgsComposerAttributeTable::setMaximumNumberOfFeatures( int features )
{
  if ( features == mMaximumNumberOfFeatures )
  {
    return;
  }

  mMaximumNumberOfFeatures = features;
  refreshAttributes();
}

void QgsComposerAttributeTable::setDisplayOnlyVisibleFeatures( bool visibleOnly )
{
  if ( visibleOnly == mShowOnlyVisibleFeatures )
  {
    return;
  }

  mShowOnlyVisibleFeatures = visibleOnly;
  refreshAttributes();
}

void QgsComposerAttributeTable::setFilterFeatures( bool filter )
{
  if ( filter == mFilterFeatures )
  {
    return;
  }

  mFilterFeatures = filter;
  refreshAttributes();
}

void QgsComposerAttributeTable::setFeatureFilter( const QString& expression )
{
  if ( expression == mFeatureFilter )
  {
    return;
  }

  mFeatureFilter = expression;
  refreshAttributes();
}

void QgsComposerAttributeTable::setDisplayAttributes( const QSet<int>& attr, bool refresh )
{
  mDisplayAttributes = attr;

  if ( refresh )
  {
    refreshAttributes();
  }
}

void QgsComposerAttributeTable::setFieldAliasMap( const QMap<int, QString>& map, bool refresh )
{
  mFieldAliasMap = map;

  if ( refresh )
  {
    refreshAttributes();
  }
}

bool QgsComposerAttributeTable::getFeatureAttributes( QList<QgsAttributeMap> &attributeMaps )
{
  if ( !mVectorLayer )
  {
    return false;
  }

  attributeMaps.clear();

  //prepare filter expression
  std::auto_ptr<QgsExpression> filterExpression;
  bool activeFilter = false;
  if ( mFilterFeatures && !mFeatureFilter.isEmpty() )
  {
    filterExpression = std::auto_ptr<QgsExpression>( new QgsExpression( mFeatureFilter ) );
    if ( !filterExpression->hasParserError() )
    {
      activeFilter = true;
    }
  }

  QgsRectangle selectionRect;
  if ( mComposerMap && mShowOnlyVisibleFeatures )
  {
    selectionRect = *mComposerMap->currentMapExtent();
    if ( mVectorLayer && mComposition->mapSettings().hasCrsTransformEnabled() )
    {
      //transform back to layer CRS
      QgsCoordinateTransform coordTransform( mVectorLayer->crs(), mComposition->mapSettings().destinationCrs() );
      try
      {
        selectionRect = coordTransform.transformBoundingBox( selectionRect, QgsCoordinateTransform::ReverseTransform );
      }
      catch ( QgsCsException &cse )
      {
        Q_UNUSED( cse );
        return false;
      }
    }
  }

  QgsFeatureRequest req;
  if ( !selectionRect.isEmpty() )
    req.setFilterRect( selectionRect );

  req.setFlags( mShowOnlyVisibleFeatures ? QgsFeatureRequest::ExactIntersect : QgsFeatureRequest::NoGeometry );

  if ( !mDisplayAttributes.isEmpty() )
    req.setSubsetOfAttributes( mDisplayAttributes.toList() );

  QgsFeature f;
  int counter = 0;
  QgsFeatureIterator fit = mVectorLayer->getFeatures( req );

  while ( fit.nextFeature( f ) && counter < mMaximumNumberOfFeatures )
  {
    //check feature against filter
    if ( activeFilter )
    {
      QVariant result = filterExpression->evaluate( &f, mVectorLayer->pendingFields() );
      // skip this feature if the filter evaluation if false
      if ( !result.toBool() )
      {
        continue;
      }
    }

    attributeMaps.push_back( QgsAttributeMap() );

    for ( int i = 0; i < f.attributes().size(); i++ )
    {
      if ( !mDisplayAttributes.isEmpty() && !mDisplayAttributes.contains( i ) )
        continue;

      attributeMaps.last().insert( i, f.attributes()[i] );
    }

    ++counter;
  }

  //sort the list, starting with the last attribute
  QgsComposerAttributeTableCompare c;
  for ( int i = mSortInformation.size() - 1; i >= 0; --i )
  {
    c.setSortColumn( mSortInformation.at( i ).first );
    c.setAscending( mSortInformation.at( i ).second );
    qStableSort( attributeMaps.begin(), attributeMaps.end(), c );
  }
  return true;
}

QMap<int, QString> QgsComposerAttributeTable::headerLabels() const
{
  QMap<int, QString> header;
  if ( mVectorLayer )
  {
    const QgsFields& vectorFields = mVectorLayer->pendingFields();
    for ( int idx = 0; idx < vectorFields.count(); ++idx )
    {
      if ( mDisplayAttributes.size() > 0 && !mDisplayAttributes.contains( idx ) )
      {
        continue;
      }
      header.insert( idx, attributeDisplayName( idx, vectorFields[idx].name() ) );
    }
  }
  return header;
}

QString QgsComposerAttributeTable::attributeDisplayName( int attributeIndex, const QString& name ) const
{
  return mFieldAliasMap.value( attributeIndex, name );
}

void QgsComposerAttributeTable::removeLayer( QString layerId )
{
  if ( mVectorLayer )
  {
    if ( layerId == mVectorLayer->id() )
    {
      mVectorLayer = 0;
    }
  }
}

void QgsComposerAttributeTable::setSceneRect( const QRectF& rectangle )
{
  double titleHeight =  2 * mGridStrokeWidth + 2 * mLineTextDistance + fontAscentMillimeters( mHeaderFont );
  double attributeHeight = mGridStrokeWidth + 2 * mLineTextDistance + fontAscentMillimeters( mContentFont );
  if (( rectangle.height() - titleHeight ) > 0 )
  {
    mMaximumNumberOfFeatures = ( rectangle.height() - titleHeight ) / attributeHeight;
  }
  else
  {
    mMaximumNumberOfFeatures = 0;
  }
  QgsComposerItem::setSceneRect( rectangle );

  //refresh table attributes, since number of features has likely changed
  refreshAttributes();

  emit maximumNumberOfFeaturesChanged( mMaximumNumberOfFeatures );
}

void QgsComposerAttributeTable::setSortAttributes( const QList<QPair<int, bool> > att, bool refresh )
{
  mSortInformation = att;

  if ( refresh )
  {
    refreshAttributes();
  }
}

bool QgsComposerAttributeTable::writeXML( QDomElement& elem, QDomDocument & doc ) const
{
  QDomElement composerTableElem = doc.createElement( "ComposerAttributeTable" );
  composerTableElem.setAttribute( "showOnlyVisibleFeatures", mShowOnlyVisibleFeatures );
  composerTableElem.setAttribute( "maxFeatures", mMaximumNumberOfFeatures );
  composerTableElem.setAttribute( "filterFeatures", mFilterFeatures ? "true" : "false" );
  composerTableElem.setAttribute( "featureFilter", mFeatureFilter );

  if ( mComposerMap )
  {
    composerTableElem.setAttribute( "composerMap", mComposerMap->id() );
  }
  else
  {
    composerTableElem.setAttribute( "composerMap", -1 );
  }
  if ( mVectorLayer )
  {
    composerTableElem.setAttribute( "vectorLayer", mVectorLayer->id() );
  }

  //display attributes
  QDomElement displayAttributesElem = doc.createElement( "displayAttributes" );
  QSet<int>::const_iterator attIt = mDisplayAttributes.constBegin();
  for ( ; attIt != mDisplayAttributes.constEnd(); ++attIt )
  {
    QDomElement attributeIndexElem = doc.createElement( "attributeEntry" );
    attributeIndexElem.setAttribute( "index", *attIt );
    displayAttributesElem.appendChild( attributeIndexElem );
  }
  composerTableElem.appendChild( displayAttributesElem );

  //alias map
  QDomElement aliasMapElem = doc.createElement( "attributeAliasMap" );
  QMap<int, QString>::const_iterator aliasIt = mFieldAliasMap.constBegin();
  for ( ; aliasIt != mFieldAliasMap.constEnd(); ++aliasIt )
  {
    QDomElement mapEntryElem = doc.createElement( "aliasEntry" );
    mapEntryElem.setAttribute( "key", aliasIt.key() );
    mapEntryElem.setAttribute( "value", aliasIt.value() );
    aliasMapElem.appendChild( mapEntryElem );
  }
  composerTableElem.appendChild( aliasMapElem );

  //sort info
  QDomElement sortColumnsElem = doc.createElement( "sortColumns" );
  QList< QPair<int, bool> >::const_iterator sortIt = mSortInformation.constBegin();
  for ( ; sortIt != mSortInformation.constEnd(); ++sortIt )
  {
    QDomElement columnElem = doc.createElement( "column" );
    columnElem.setAttribute( "index", QString::number( sortIt->first ) );
    columnElem.setAttribute( "ascending", sortIt->second ? "true" : "false" );
    sortColumnsElem.appendChild( columnElem );
  }
  composerTableElem.appendChild( sortColumnsElem );
  elem.appendChild( composerTableElem );
  bool ok = tableWriteXML( composerTableElem, doc );
  return ok;
}

bool QgsComposerAttributeTable::readXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  if ( itemElem.isNull() )
  {
    return false;
  }

  mShowOnlyVisibleFeatures = itemElem.attribute( "showOnlyVisibleFeatures", "1" ).toInt();
  mFilterFeatures = itemElem.attribute( "filterFeatures", "false" ) == "true" ? true : false;
  mFeatureFilter = itemElem.attribute( "featureFilter", "" );

  //composer map
  int composerMapId = itemElem.attribute( "composerMap", "-1" ).toInt();
  if ( composerMapId == -1 )
  {
    mComposerMap = 0;
  }

  if ( composition() )
  {
    mComposerMap = composition()->getComposerMapById( composerMapId );
  }
  else
  {
    mComposerMap = 0;
  }

  if ( mComposerMap )
  {
    //if we have found a valid map item, listen out to extent changes on it and refresh the table
    QObject::connect( mComposerMap, SIGNAL( extentChanged() ), this, SLOT( refreshAttributes() ) );
  }

  //vector layer
  QString layerId = itemElem.attribute( "vectorLayer", "not_existing" );
  if ( layerId == "not_existing" )
  {
    mVectorLayer = 0;
  }
  else
  {
    QgsMapLayer* ml = QgsMapLayerRegistry::instance()->mapLayer( layerId );
    if ( ml )
    {
      mVectorLayer = dynamic_cast<QgsVectorLayer*>( ml );
      if ( mVectorLayer )
      {
        //if we have found a valid vector layer, listen for modifications on it and refresh the table
        QObject::connect( mVectorLayer, SIGNAL( layerModified() ), this, SLOT( refreshAttributes() ) );
      }
    }
  }

  //restore display attribute map
  mDisplayAttributes.clear();
  QDomNodeList displayAttributeList = itemElem.elementsByTagName( "displayAttributes" );
  if ( displayAttributeList.size() > 0 )
  {
    QDomElement displayAttributesElem =  displayAttributeList.at( 0 ).toElement();
    QDomNodeList attributeEntryList = displayAttributesElem.elementsByTagName( "attributeEntry" );
    for ( int i = 0; i < attributeEntryList.size(); ++i )
    {
      QDomElement attributeEntryElem = attributeEntryList.at( i ).toElement();
      int index = attributeEntryElem.attribute( "index", "-1" ).toInt();
      if ( index != -1 )
      {
        mDisplayAttributes.insert( index );
      }
    }
  }

  //restore alias map
  mFieldAliasMap.clear();
  QDomNodeList aliasMapNodeList = itemElem.elementsByTagName( "attributeAliasMap" );
  if ( aliasMapNodeList.size() > 0 )
  {
    QDomElement attributeAliasMapElem = aliasMapNodeList.at( 0 ).toElement();
    QDomNodeList aliasMepEntryList = attributeAliasMapElem.elementsByTagName( "aliasEntry" );
    for ( int i = 0; i < aliasMepEntryList.size(); ++i )
    {
      QDomElement aliasEntryElem = aliasMepEntryList.at( i ).toElement();
      int key = aliasEntryElem.attribute( "key", "-1" ).toInt();
      QString value = aliasEntryElem.attribute( "value", "" );
      mFieldAliasMap.insert( key, value );
    }
  }

  //restore sort columns
  mSortInformation.clear();
  QDomElement sortColumnsElem = itemElem.firstChildElement( "sortColumns" );
  if ( !sortColumnsElem.isNull() )
  {
    QDomNodeList columns = sortColumnsElem.elementsByTagName( "column" );
    for ( int i = 0; i < columns.size(); ++i )
    {
      QDomElement columnElem = columns.at( i ).toElement();
      int attribute = columnElem.attribute( "index" ).toInt();
      bool ascending = columnElem.attribute( "ascending" ) == "true" ? true : false;
      mSortInformation.push_back( qMakePair( attribute, ascending ) );
    }
  }
  bool success = tableReadXML( itemElem, doc );

  //must be done here because tableReadXML->setSceneRect changes mMaximumNumberOfFeatures
  mMaximumNumberOfFeatures = itemElem.attribute( "maxFeatures", "5" ).toInt();

  refreshAttributes();

  emit itemChanged();
  return success;
}
