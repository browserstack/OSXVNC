//
//  copyrect.c
//  OSXvnc
//
//  Created by Shirshendu Mukherjee on 15/01/13.
//
//

#include <stdio.h>
#include <pthread.h>
#include "rfb.h"

/*
 * Returns the pixel offset by which a vertical scroll has taken place
 * Returns zero if no scroll could be detected
 *
 */

static int detectVerticalScroll(int w){
    rfbDebugLog("Detecting vertical scroll now");
    //if(w > 800)
    //    return 40;
    //else
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

Bool rfbSendRectEncodingCopyRect(rfbClientPtr cl, int* x, int* y, int* w, int* h)
{
    
    rfbDebugLog("Starting copyrect processing");
    
    int scrolledPixels = detectVerticalScroll(*w);
    
    if (scrolledPixels == 0)
        return FALSE;

    rfbCopyRect cp;
    cp.srcX = Swap16IfLE(*x);
    cp.srcY = Swap16IfLE(*y + scrolledPixels);
    //rfbDebugLog("width: %d, height: %d, src_x: %d, src_y: %d", *w, *h, *x, *y + scrolledPixels);
    if (!sendCopyRect(cl, *x, *y, *w, *h - abs(scrolledPixels), cp))
        return FALSE;
    //if (!sendCopyRect(cl, *x, *y, *w, *h - abs(scrolledPixels), *x, *y + scrolledPixels))
    //    return FALSE;
    // Remove copyrected rectangle from modified region
    // *x does not change
    if(scrolledPixels>0){
        *y = *y + *h - scrolledPixels;
    }
    else {
        // *y remains the same
    }
    // *w does not change
    *h = abs(scrolledPixels);

    return TRUE;
}

