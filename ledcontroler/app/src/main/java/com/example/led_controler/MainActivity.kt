package com.example.led_controler

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.SeekBar
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat

class MainActivity : AppCompatActivity() {
    private val TAG = "EMI_APP"

    // UUID muszą się zgadzać z tymi na ESP32
    private val SERVICE_UUID = java.util.UUID.fromString("0000FFE0-0000-1000-8000-00805F9B34FB")
    private val CHAR_UUID    = java.util.UUID.fromString("0000FFE1-0000-1000-8000-00805F9B34FB")

    private var bluetoothGatt: BluetoothGatt? = null
    private var charWrite: BluetoothGattCharacteristic? = null
    private var isConnected = false

    private lateinit var seekR: SeekBar
    private lateinit var seekG: SeekBar
    private lateinit var seekB: SeekBar
    private lateinit var colorPreview: View
    private lateinit var btnConnect: Button

    @SuppressLint("MissingPermission")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btnConnect = findViewById(R.id.btnConnect)
        seekR = findViewById(R.id.seekR)
        seekG = findViewById(R.id.seekG)
        seekB = findViewById(R.id.seekB)
        colorPreview = findViewById(R.id.colorPreview)

        // Prośba o uprawnienia
        ActivityCompat.requestPermissions(this, arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.ACCESS_FINE_LOCATION
        ), 1)

        btnConnect.setOnClickListener {
            if (isConnected) {
                bluetoothGatt?.disconnect()
                btnConnect.text = "Połącz z EMI-LED"
            } else {
                startScan()
                btnConnect.text = "Szukanie..."
            }
        }

        val seekListener = object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                updateColorPreview()
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                sendColor(seekR.progress, seekG.progress, seekB.progress)
            }
        }
        seekR.setOnSeekBarChangeListener(seekListener)
        seekG.setOnSeekBarChangeListener(seekListener)
        seekB.setOnSeekBarChangeListener(seekListener)
    }

    private fun updateColorPreview() {
        val color = Color.rgb(seekR.progress, seekG.progress, seekB.progress)
        colorPreview.setBackgroundColor(color)
    }

    @SuppressLint("MissingPermission")
    private fun startScan() {
        val adapter = (getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter
        if (!adapter.isEnabled) {
            Toast.makeText(this, "Włącz Bluetooth!", Toast.LENGTH_SHORT).show()
            return
        }

        adapter.bluetoothLeScanner.startScan(object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                if (result.device.name == "EMI-LED") {
                    adapter.bluetoothLeScanner.stopScan(this)
                    Log.d(TAG, "Znaleziono EMI-LED! Łączę...")
                    runOnUiThread { btnConnect.text = "Łączenie..." }
                    bluetoothGatt = result.device.connectGatt(this@MainActivity, false, gattCallback)
                }
            }
        })
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(TAG, "Połączono! Odkrywam usługi...")
                gatt.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                isConnected = false
                runOnUiThread { btnConnect.text = "Połącz z EMI-LED" }
            }
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            val service = gatt.getService(SERVICE_UUID)
            if (service != null) {
                charWrite = service.getCharacteristic(CHAR_UUID)
                if (charWrite != null) {
                    isConnected = true
                    runOnUiThread {
                        Toast.makeText(this@MainActivity, "Połączono z EMI-LED!", Toast.LENGTH_SHORT).show()
                        btnConnect.text = "Rozłącz"
                    }
                    Log.d(TAG, "Usługi odkryte, można wysyłać dane.")
                }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun sendColor(r: Int, g: Int, b: Int) {
        if (!isConnected || bluetoothGatt == null || charWrite == null) return

        val data = byteArrayOf(r.toByte(), g.toByte(), b.toByte())

        // Nowe API dla Androida 13+ vs starsze
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            bluetoothGatt?.writeCharacteristic(charWrite!!, data, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            charWrite?.value = data
            @Suppress("DEPRECATION")
            bluetoothGatt?.writeCharacteristic(charWrite)
        }
        Log.d(TAG, "Wysłano kolor: R=$r, G=$g, B=$b")
    }
}