//
//  copyrect.c
//  OSXvnc
//
//  Created by Shirshendu Mukherjee on 15/01/13.
//
//

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rfb.h"

#define DEBUG_MASK_COPYRECT FALSE

/*
 * Returns the pixel offset by which a vertical scroll has taken place
 * Returns zero if no scroll could be detected
 *
 */

static int detectVerticalScroll(char * currentFb, rfbClientPtr cl, int fbWidthBytes, int fbHeight, int modRegionX, int modRegionY, int modifiedWidth, int modifiedHeight){
    
    //rfbDebugLog("Detecting vertical scroll now");
    int bytesPerPixel = rfbScreen.bitsPerPixel/8;
    int testScroll = 0;
    char * cachedFb = cl->scalingFrameBufferCache;
    rfbDebugLog("starting cmp");
    //AAAAAA//
    //////////////////////////////
    if(DEBUG_MASK_COPYRECT){
        modifiedWidth--;
        modifiedWidth--;
        modifiedWidth--;
        modRegionX++;
        for(testScroll=40;testScroll<=40;testScroll++){
            int x=0;
            
            for(int i=0;i<modifiedHeight - testScroll;i++){
                //if (x==0){
                for(int j=bytesPerPixel;j<bytesPerPixel * modifiedWidth;j++){
                    //x = memcmp(cachedFb + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + ((i+testScroll) * fbWidthBytes), currentFb+ modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + i * fbWidthBytes, (size_t)1);
                    //if (0<=i<=14 && (0<=j<bytesPerPixel || j==modifiedWidth)){
                    //    continue;
                    //}
                    if ( *(cachedFb + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + ((i+testScroll) * fbWidthBytes) + j) == *(cl->realFrameBuffer + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + i * fbWidthBytes + j)){
                        *(currentFb+ modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + i * fbWidthBytes + j) = 0x00;
                    }
                    else {
                        *(currentFb+ modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + i * fbWidthBytes + j) = 0xAA;
                        x=1;
                    }
                    
                    //}
                }
            }
            if(x==0){
                rfbDebugLog("masked scroll: %d",testScroll);
                return 0;}
            x=0;
            for(int i=0;i<modifiedHeight - testScroll;i++){
                if (x==0){
                    x = memcmp(cachedFb + modRegionY * fbWidthBytes + i * fbWidthBytes + modRegionX * bytesPerPixel, currentFb + ((i+testScroll) * fbWidthBytes) + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel, (size_t)(bytesPerPixel * modifiedWidth));
                }
            }
            //    if(x==0)
            //        return (-1 * testScroll);
            
        }
    }
    else {
        ///////////////////////////////
        
        
        modifiedWidth--;
        modifiedWidth--;
        modifiedWidth--;
        modRegionX++;
        for(testScroll=0;testScroll<=modifiedHeight-40;testScroll++){
            int x=0;
            //
            for(int i=0;i<modifiedHeight - testScroll;i++){
                if (x==0){
                    x = memcmp(cachedFb + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + ((i+testScroll) * fbWidthBytes), currentFb+ modRegionY * fbWidthBytes + modRegionX * bytesPerPixel + i * fbWidthBytes, (size_t)(bytesPerPixel * modifiedWidth));
                }
            }
            if(x==0){
                if (testScroll != 0)
                    return testScroll;
                else
                    return 12345;
            }
            
            x=0;
            for(int i=0;i<modifiedHeight - testScroll;i++){
                if (x==0){
                    x = memcmp(cachedFb + modRegionY * fbWidthBytes + i * fbWidthBytes + modRegionX * bytesPerPixel, currentFb + ((i+testScroll) * fbWidthBytes) + modRegionY * fbWidthBytes + modRegionX * bytesPerPixel, (size_t)(bytesPerPixel * modifiedWidth));
                }
            }
            if(x==0)
                return (-1 * testScroll);
        }
    }
    ///*///
    rfbDebugLog("not a scroll");
    return 0;
}

/*
 * Sends copyrect headers
 */
static Bool sendCopyRectHeader(cl, x, y, w, h)
rfbClientPtr cl;
int x, y, w, h;
{
    rfbFramebufferUpdateRectHeader rect;
    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }
    // Set up headers
    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingCopyRect);
    
    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;
    cl->rfbRectanglesSent[rfbEncodingCopyRect]++;
    cl->rfbBytesSent[rfbEncodingCopyRect] += sz_rfbFramebufferUpdateRectHeader;
    return TRUE;
}



/*
 * Dispatches the copyrect encoding to the client
 */

static Bool sendCopyRect(rfbClientPtr cl, int x, int y, int w, int h, rfbCopyRect cp){
    // source rectangle x, y, w, h, destination point
    rfbFramebufferUpdateRectHeader rect;
    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }
    
    if(!sendCopyRectHeader(cl, x, y, w, h)){
        return FALSE;
    }
    memcpy(&cl->updateBuf[cl->ublen], (char *)&cp,
           sz_rfbCopyRect);
    cl->ublen += sz_rfbCopyRect;
    
    //    if(!rfbSendUpdateBuf(cl))
    //        return FALSE;
    
    return TRUE;
}

/*
 * Processes and sends vertical scrolls, if any.
 * Updates the screen's modified region accordingly, so that further
 * encodings would have to process that much less data.
 *
 */

Bool rfbSendRectEncodingCopyRect(rfbClientPtr cl, int x, int y, int w, int h, RegionPtr updateRegion)
{
    const unsigned long csh = (rfbScreen.height+cl->scalingFactor-1)/ cl->scalingFactor;
    const unsigned long csw = (rfbScreen.width +cl->scalingFactor-1)/ cl->scalingFactor;
    // get current FB
    char *fbptr = (cl->scalingFrameBuffer + (cl->scalingPaddedWidthInBytes * 0)
                   + (0 * (rfbScreen.bitsPerPixel / 8)));
    
    memcpy(cl->realFrameBuffer, fbptr, (size_t)(csh * cl->scalingPaddedWidthInBytes));
    
    // no copyrect for regions <600px width, 40 px height
    if (w < 600 || cl->firstCopyRect){
        //for(int i = y; i < y+h; i++){
        //    memcpy(cl->scalingFrameBufferCache + x * rfbScreen.bitsPerPixel/8 + i * cl->scalingPaddedWidthInBytes, cl->realFrameBuffer + x * rfbScreen.bitsPerPixel/8 + i * cl->scalingPaddedWidthInBytes, (size_t)(w * rfbScreen.bitsPerPixel/8));
        //}
        rfbDebugLog("chota update %d %d %d %d", x,y,w,h);
        memcpy(cl->scalingFrameBufferCache, cl->realFrameBuffer, (size_t)(csh * cl->scalingPaddedWidthInBytes));
        //rfbDebugLog("fcr: %d %d %d %d", x,y,w,h);
        cl->firstCopyRect = FALSE;
        return 0;
    }

    // detect scroll
    int scrolledPixels = detectVerticalScroll(fbptr, cl, cl->scalingPaddedWidthInBytes, csh, x, y, w - 20, h - 5);
    rfbDebugLog("scroll: %d, width: %d, height: %d", scrolledPixels, w, h);
    // cache current FB for next time.
    memcpy(cl->scalingFrameBufferCache, cl->realFrameBuffer, (size_t)(csh * cl->scalingPaddedWidthInBytes));
    
    
    if (scrolledPixels == 0 || scrolledPixels == 12345)
        return FALSE;
    
    
    rfbCopyRect cp;
    cp.srcX = Swap16IfLE(x);
    cp.srcY = (scrolledPixels > 0) ? Swap16IfLE(y + scrolledPixels) : Swap16IfLE(y);
    //rfbDebugLog("width: %d, height: %d, src_x: %d, src_y: %d", w, h, x, y + scrolledPixels);
    if (scrolledPixels > 0){
        if (!sendCopyRect(cl, x, y, w - 20, h - abs(scrolledPixels) - 5, cp))
            return FALSE;
    }
    else {
        if (!sendCopyRect(cl, x, y + abs(scrolledPixels), w - 20, h - abs(scrolledPixels) - 5, cp))
            return FALSE;
    }

    // Remove copyrected rectangle from modified region
    xRectangle copyRects[1];
    
    copyRects[0].x = x;
    copyRects[0].width = w - 20;
    copyRects[0].height = h - 5 - abs(scrolledPixels);
    if(scrolledPixels > 0){
        copyRects[0].y = y;
    }
    else{
        copyRects[0].y = y + abs(scrolledPixels);
    }

    REGION_SUBTRACT(pScreen, updateRegion, updateRegion, RECTS_TO_REGION(pScreen, 1, copyRects, CT_NONE));
    return TRUE;
}

