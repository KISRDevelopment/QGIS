/***************************************************************************
                              qgscomposerhtml.h
    ------------------------------------------------------------
    begin                : July 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSCOMPOSERHTML_H
#define QGSCOMPOSERHTML_H

#include "qgscomposermultiframe.h"
#include <QUrl>

class QWebPage;
class QImage;

class CORE_EXPORT QgsComposerHtml: public QgsComposerMultiFrame
{
    Q_OBJECT
  public:
    QgsComposerHtml( QgsComposition* c, bool createUndoCommands );
    QgsComposerHtml();
    ~QgsComposerHtml();

    void setUrl( const QUrl& url );
    const QUrl& url() const { return mUrl; }

    QSizeF totalSize() const;
    void render( QPainter* p, const QRectF& renderExtent );

    bool writeXML( QDomElement& elem, QDomDocument & doc, bool ignoreFrames = false ) const;
    bool readXML( const QDomElement& itemElem, const QDomDocument& doc, bool ignoreFrames = false );

    void addFrame( QgsComposerFrame* frame, bool recalcFrameSizes = true );

    //overriden to break frames without dividing lines of text
    double findNearbyPageBreak( double yPos );

    /**Returns whether html item is using smart breaks. Smart breaks prevent
     * the html frame contents from breaking mid-way though a line of text.
     * @returns true if html item is using smart breaks
     * @see setUseSmartBreaks
     */
    bool useSmartBreaks() const { return mUseSmartBreaks; }

    /**Sets whether the html item should use smart breaks. Smart breaks prevent
     * the html frame contents from breaking mid-way though a line of text.
     * @param useSmartBreaks set to true to prevent content from breaking
     * mid-way through a line of text
     * @see useSmartBreaks
     */
    void setUseSmartBreaks( bool useSmartBreaks );

  private slots:
    void frameLoaded( bool ok );

  private:
    QUrl mUrl;
    QWebPage* mWebPage;
    bool mLoaded;
    QSizeF mSize; //total size in mm
    double mHtmlUnitsToMM;
    QImage* mRenderedPage;
    bool mUseSmartBreaks;

    double htmlUnitsToMM(); //calculate scale factor

    //renders a snapshot of the page to a cached image
    void renderCachedImage();
};

#endif // QGSCOMPOSERHTML_H
