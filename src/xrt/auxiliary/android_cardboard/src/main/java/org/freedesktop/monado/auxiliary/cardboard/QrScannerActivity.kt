// Copyright 2024-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  QR code scanner activity for Google Cardboard codes.
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

package org.freedesktop.monado.auxiliary.cardboard

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import de.markusfisch.android.barcodescannerview.widget.BarcodeScannerView
import org.freedesktop.monado.auxiliary.cardboard.databinding.ActivityScannerBinding

class QrScannerActivity : AppCompatActivity() {
    private lateinit var viewBinding: ActivityScannerBinding
    private lateinit var scannerView: BarcodeScannerView

    companion object {
        private const val TAG = "MonadoQrScanner"
        private const val PERMISSION_REQUEST_CODE = 200
    }

    private fun tryGetPermissions(): Boolean {

        if (
            ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) ==
                PackageManager.PERMISSION_GRANTED
        ) {
            Log.i(TAG, "Camera permission has been granted")
            return true
        }
        Log.i(TAG, "Camera permission has not been granted, requesting")
        val permissionStrings = arrayOf(Manifest.permission.CAMERA)
        ActivityCompat.requestPermissions(this, permissionStrings, PERMISSION_REQUEST_CODE)
        return false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        viewBinding = ActivityScannerBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)
        if (tryGetPermissions()) {
            startScanner()
        }
    }

    private fun startScanner() {
        scannerView = findViewById(R.id.barcode_scanner)
        scannerView.cropRatio = .75f

        scannerView.setOnBarcodeListener { result -> processScan(result.text) }
    }

    public override fun onResume() {
        super.onResume()
        if (this::scannerView.isInitialized) {

            scannerView.openAsync()
        } else {
            tryGetPermissions()
        }
    }

    public override fun onPause() {
        super.onPause()
        if (this::scannerView.isInitialized) {
            scannerView.close()
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        Log.i(TAG, "onRequestPermissionResult")
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                startScanner()
            }
        }
    }

    private fun processScan(value: String): Boolean {
        val uri = qrCodeValueToCardboardURI(value)
        if (uri == null) {
            Log.e(TAG, "url is not cardboard config")
            runOnUiThread { Toast.makeText(this, "QR code invalid", Toast.LENGTH_SHORT).show() }
            return true
        }

        if (isOriginalCardboard(uri)) {
            // This is an approximation of the original Cardboard config - only 2 digits for
            // distortion, not 3, but good enough.
            Log.d(TAG, "QR code is for Cardboard V1 with no parameters in it, using default")
            saveCardboardV1Params(this)
        } else {
            saveCardboardParamsFromResolvedUri(this, uri)
        }

        Log.d(TAG, "QR code valid")
        runOnUiThread {
            Toast.makeText(applicationContext, "Cardboard parameters saved", Toast.LENGTH_SHORT)
                .show()
        }
        finish()
        return false
    }
}
