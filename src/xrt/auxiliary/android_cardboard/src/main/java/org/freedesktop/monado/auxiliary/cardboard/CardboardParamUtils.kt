// Copyright 2024-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Utilities for Google Cardboard codes.
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

package org.freedesktop.monado.auxiliary.cardboard

import android.content.Context
import android.net.Uri
import android.util.Base64
import android.util.Log
import androidx.core.net.toUri
import java.io.File
import java.io.FileOutputStream
import java.net.HttpURLConnection
import java.net.URL

private const val TAG = "CardboardParamUtils"

@Suppress("SpellCheckingInspection")
private const val cardboardV1ParamValue: String =
    "CgxHb29nbGUsIEluYy4SDENhcmRib2FyZCB2MR0xCCw9JY_CdT0qEAAAIEIAACBCAAAgQgAAIEJYADUpXA89OgiuR-E-mpkZPlABYAE"
private const val googleDomain = "google.com"

private const val cardboardConfigPath = "/cardboard/cfg"

private const val cardboardV1EquivUri =
    "https://$googleDomain$cardboardConfigPath?p=$cardboardV1ParamValue"

/** Cardboard V1 had a QR code with no parameters encoded. */
fun isOriginalCardboard(uri: Uri): Boolean {
    return uri.authority.equals("g.co") && uri.path.equals("/cardboard")
}

/** Newer Cardboard-compatible devices have parameters encoded in a URI (eventually) */
fun isCardboardParamUri(uri: Uri): Boolean {
    return uri.authority.equals(googleDomain) && uri.path.equals(cardboardConfigPath)
}

/** Cardboard URIs include those with params and the original human-targeted Cardboard V1 URI. */
fun isCardboardUri(uri: Uri): Boolean {
    return isOriginalCardboard(uri) || isCardboardParamUri(uri)
}

/**
 * Some QR codes are missing the scheme. Further, we do not want to visit plain http URIs looking
 * for a redirect, therefore we replace the scheme if applicable.
 */
private fun sanitizeAndParseUri(value: String): Uri {
    return if (!value.startsWith("http")) {
        // missing protocol
        "https://$value".toUri()
    } else if (value.startsWith("http://")) {
        // non-https
        value.replaceFirst("http", "https").toUri()
    } else {
        value.toUri()
    }
}

/** Save current_device_params with approximate Cardboard V1 params */
fun saveCardboardV1Params(qrScannerActivity: QrScannerActivity) {
    saveCardboardParamsFromResolvedUri(qrScannerActivity, cardboardV1EquivUri.toUri())
}

/** Save current_device_params from a Cardboard param URI. */
fun saveCardboardParamsFromResolvedUri(context: Context, uri: Uri) {
    assert(isCardboardParamUri(uri))
    val paramEncoded = uri.getQueryParameter("p")
    val flags = Base64.URL_SAFE or Base64.NO_WRAP or Base64.NO_PADDING
    val data = Base64.decode(paramEncoded, flags)

    val configFile = File(context.filesDir, "current_device_params")
    configFile.createNewFile()

    Log.d(TAG, "Saving to file $configFile")
    FileOutputStream(configFile).use { it.write(data) }
}

/**
 * Given some data from a QR Code, try to get to a Cardboard URI: either the original Cardboard web
 * site, or a parameter URI.
 *
 * @return null if we could not get to a Cardboard URI even after several redirects.
 */
fun qrCodeValueToCardboardURI(value: String): Uri? {

    Log.i(TAG, "QR code scan data $value")

    var uri = sanitizeAndParseUri(value)

    Log.i(TAG, "QR code URL $uri")

    // Try fetching the URL, following redirects, repeatedly.
    for (i in 0..5) {
        if (isCardboardUri(uri)) {
            // got one!
            break
        }
        val connection = URL(uri.toString()).openConnection() as HttpURLConnection
        connection.setRequestMethod("HEAD")
        connection.instanceFollowRedirects = false
        connection.connect()
        val code = connection.getResponseCode()
        if (
            code == HttpURLConnection.HTTP_MOVED_PERM || code == HttpURLConnection.HTTP_MOVED_TEMP
        ) {
            // Sanitize: we only consider visiting https, for privacy
            uri = sanitizeAndParseUri(connection.getHeaderField("Location"))

            Log.i(TAG, "followed url to $uri")
        } else {
            // was not a redirect, all done
            break
        }
    }

    Log.i(TAG, "ending with url $uri")

    if (!isCardboardUri(uri)) {
        Log.e(TAG, "url is not cardboard config")
        return null
    }
    return uri
}
