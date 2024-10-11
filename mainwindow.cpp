/*
 * Copyright (C) 2024, Danijel Sipos, danijel@simarine.net
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QTcpSocket>
#include <QThread>

QByteArray dataArr;

#define PACK_SIZE 4162

uint16_t crc_table[16] {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};

uint16_t crctable[256] {
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
    0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x0919, 0x1890, 0x2A0B, 0x3B82, 0x4F3D, 0x5EB4, 0x6C2F, 0x7DA6,
    0x8551, 0x94D8, 0xA643, 0xB7CA, 0xC375, 0xD2FC, 0xE067, 0xF1EE,
    0x1232, 0x03BB, 0x3120, 0x20A9, 0x5416, 0x459F, 0x7704, 0x668D,
    0x9E7A, 0x8FF3, 0xBD68, 0xACE1, 0xD85E, 0xC9D7, 0xFB4C, 0xEAC5,
    0x1B2B, 0x0AA2, 0x3839, 0x29B0, 0x5D0F, 0x4C86, 0x7E1D, 0x6F94,
    0x9763, 0x86EA, 0xB471, 0xA5F8, 0xD147, 0xC0CE, 0xF255, 0xE3DC,
    0x2464, 0x35ED, 0x0776, 0x16FF, 0x6240, 0x73C9, 0x4152, 0x50DB,
    0xA82C, 0xB9A5, 0x8B3E, 0x9AB7, 0xEE08, 0xFF81, 0xCD1A, 0xDC93,
    0x2D7D, 0x3CF4, 0x0E6F, 0x1FE6, 0x6B59, 0x7AD0, 0x484B, 0x59C2,
    0xA135, 0xB0BC, 0x8227, 0x93AE, 0xE711, 0xF698, 0xC403, 0xD58A,
    0x3656, 0x27DF, 0x1544, 0x04CD, 0x7072, 0x61FB, 0x5360, 0x42E9,
    0xBA1E, 0xAB97, 0x990C, 0x8885, 0xFC3A, 0xEDB3, 0xDF28, 0xCEA1,
    0x3F4F, 0x2EC6, 0x1C5D, 0x0DD4, 0x796B, 0x68E2, 0x5A79, 0x4BF0,
    0xB307, 0xA28E, 0x9015, 0x819C, 0xF523, 0xE4AA, 0xD631, 0xC7B8,
    0x48C8, 0x5941, 0x6BDA, 0x7A53, 0x0EEC, 0x1F65, 0x2DFE, 0x3C77,
    0xC480, 0xD509, 0xE792, 0xF61B, 0x82A4, 0x932D, 0xA1B6, 0xB03F,
    0x41D1, 0x5058, 0x62C3, 0x734A, 0x07F5, 0x167C, 0x24E7, 0x356E,
    0xCD99, 0xDC10, 0xEE8B, 0xFF02, 0x8BBD, 0x9A34, 0xA8AF, 0xB926,
    0x5AFA, 0x4B73, 0x79E8, 0x6861, 0x1CDE, 0x0D57, 0x3FCC, 0x2E45,
    0xD6B2, 0xC73B, 0xF5A0, 0xE429, 0x9096, 0x811F, 0xB384, 0xA20D,
    0x53E3, 0x426A, 0x70F1, 0x6178, 0x15C7, 0x044E, 0x36D5, 0x275C,
    0xDFAB, 0xCE22, 0xFCB9, 0xED30, 0x998F, 0x8806, 0xBA9D, 0xAB14,
    0x6CAC, 0x7D25, 0x4FBE, 0x5E37, 0x2A88, 0x3B01, 0x099A, 0x1813,
    0xE0E4, 0xF16D, 0xC3F6, 0xD27F, 0xA6C0, 0xB749, 0x85D2, 0x945B,
    0x65B5, 0x743C, 0x46A7, 0x572E, 0x2391, 0x3218, 0x0083, 0x110A,
    0xE9FD, 0xF874, 0xCAEF, 0xDB66, 0xAFD9, 0xBE50, 0x8CCB, 0x9D42,
    0x7E9E, 0x6F17, 0x5D8C, 0x4C05, 0x38BA, 0x2933, 0x1BA8, 0x0A21,
    0xF2D6, 0xE35F, 0xD1C4, 0xC04D, 0xB4F2, 0xA57B, 0x97E0, 0x8669,
    0x7787, 0x660E, 0x5495, 0x451C, 0x31A3, 0x202A, 0x12B1, 0x0338,
    0xFBCF, 0xEA46, 0xD8DD, 0xC954, 0xBDEB, 0xAC62, 0x9EF9, 0x8F70
};

class simarineWifi {
public:
    uint16_t CalculateCRC16(QByteArray data, uint16_t len)
    {
        uint16_t crc = 0, index = 5;
        while (index < len - 1) {
            crc = (crc << 8) ^ crctable[(crc >> 8) ^ (uint8_t)data[index++]];
        }

        return crc;
    }

    uint16_t GetCRC(QByteArray data)
    {
        uint16_t i;
        uint16_t crc = 0, index = 0;
        int len;

        len = data.size() - 2;
        while (len > 0) {
            len--;
            i = (uint16_t)((crc >> 12) ^ (data[index] >> 4));
            crc = (uint16_t)(crc_table[i & 0x0F] ^ (crc << 4));
            i = (uint16_t)((crc >> 12) ^ (data[index] >> 0));
            crc = (uint16_t)(crc_table[i & 0x0F] ^ (crc << 4));
            index++;
        }

        return ((uint16_t)(crc & 0xFFFF));
    }

    /*********************** AddLong ****************************
             * Parameteri
             *  id - ID ukaza
             *  index - pozicija
             *  buffer - ukazni bufer
             *  data - podatek
             ************************************************************* */
    void AddLong(uint8_t id, uint16_t* index, QByteArray& buffer, int32_t data)
    {
        buffer[(*index)++] = 0xFF; //RFU
        buffer[(*index)++] = id;
        buffer[(*index)++] = 1; // tip podatka long
        buffer[(*index)++] = (uint8_t)(data >> 24 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data >> 16 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data >> 8 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data & 0x000000FF);
    }

    void AddTimeLong(uint8_t id, uint16_t* index, QByteArray& buffer, int32_t data, int32_t time)
    {
        buffer[(*index)++] = 0xFF; //RFU
        buffer[(*index)++] = id;
        buffer[(*index)++] = 3; // tip podatka long with timestamp
        buffer[(*index)++] = (uint8_t)(time >> 24 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(time >> 16 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(time >> 8 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(time & 0x000000FF);
        buffer[(*index)++] = 0xFF; // ločilni
        buffer[(*index)++] = (uint8_t)(data >> 24 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data >> 16 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data >> 8 & 0x000000FF);
        buffer[(*index)++] = (uint8_t)(data & 0x000000FF);
    }

    /*********************** AddString ****************************
      * Parameteri
      *  id - ID ukaza
      *  index - pozicija
      *  buffer - ukazni bufer
      *  data - podatek
      ************************************************************* */
    void AddString(uint8_t id, uint16_t* index, QByteArray& buffer, std::string data)
    {
        //byte[] temp = ASCIIEncoding.ASCII.GetBytes(data);
        buffer[(*index)++] = 0xFF; //RFU
        buffer[(*index)++] = id;
        buffer[(*index)++] = 2; // tip podatka string
        for (uint8_t i = 0; i < data.length(); i++) {
            buffer[(*index)++] = data[i];
        }
        buffer[(*index)++] = 0;
    }
    /*
    byte AddTimeString(byte id, ref UInt16 index, byte[] buffer, string data, long time)
    {
        byte[] temp = ASCIIEncoding.ASCII.GetBytes(data);
        buffer[index++] = 0xFF;     //RFU
        buffer[index++] = id;
        buffer[index++] = 4;        // tip podatka string with timestamp
        buffer[index++] = (byte)(time >> 24 & 0x000000FF);
        buffer[index++] = (byte)(time >> 16 & 0x000000FF);
        buffer[index++] = (byte)(time >> 8 & 0x000000FF);
        buffer[index++] = (byte)(time & 0x000000FF);
        buffer[index++] = 0xFF;     // ločilni
        foreach (byte element in temp)
        {
            buffer[index++] = element;
        }
        buffer[index++] = 0;
        return 0;
    }
    */

    void AddWord(uint16_t index, QByteArray& buffer, uint16_t data)
    {
        buffer[index++] = (uint8_t)(data >> 8 & 0x00FF);
        buffer[index] = (uint8_t)(data & 0x00FF);
    }

    void CommandHeader(uint8_t id, int timestamp, QByteArray& data, uint16_t* index)
    {
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
        data[4] = 0; // NULL karakterji
        data[5] = 0xFF;
        data[6] = (uint8_t)id; // koda ukaza
        data[7] = (uint8_t)((timestamp & 0xFF000000) >> 24);
        data[8] = (uint8_t)((timestamp & 0x00FF0000) >> 16);
        data[9] = (uint8_t)((timestamp & 0x0000FF00) >> 8);
        data[10] = (uint8_t)(timestamp & 0x000000FF); // timestamp
        *index = 13;
    }

    void CommandFooter(QByteArray& data, uint16_t* index)
    {
        data[(*index)++] = 0xFF; // ločilni karakter
        AddWord(11, data, (*index - 11)); //nastavi dolžino
        AddWord(*index, data, CalculateCRC16(data, *index));
        (*index)++;
        (*index)++;
    }

    int waitForACK(QTcpSocket* _pSocket, int n_bytes, int sec)
    {
        qint64 bAvail;
        int cnt = 0;
        do {
            _pSocket->waitForReadyRead(1000);
            bAvail = _pSocket->bytesAvailable();
            //qDebug() << "Bytes" << bAvail;
            if (++cnt > sec)
                return -1;
        } while (bAvail < n_bytes);

        //qDebug() << "Received bytes";
        _pSocket->read(n_bytes); // READ 1st command respond

        return 0;
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QFile file(":/fonts/fonts/Roboto-Regular.ttf");
    qDebug() << file.exists();

    qDebug() << QFontDatabase::addApplicationFont(":/fonts/fonts/Roboto-Regular.ttf");
    QFont Roboto("Roboto", -1, QFont::Light);
    Roboto.setPointSize(16);
    ui->label_2->setFont(Roboto);
    Roboto.setPointSize(10);
    ui->pushButton_Browse->setFont(Roboto);
    Roboto.setPointSize(10);
    ui->pushButton->setFont(Roboto);
    Roboto.setPointSize(9);
    ui->progressBar->setFont(Roboto);
    Roboto.setPointSize(10);
    ui->lineEdit_Path->setFont(Roboto);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QMessageBox messageBox;
    simarineWifi objSwifi;
    int Filesize = 0;
    uint16_t ind = 0;
    int32_t timestamp = QDateTime::currentMSecsSinceEpoch();
    QString pathName = ui->lineEdit_Path->text();
    QFile fileToRead(pathName);
    QDataStream s(&fileToRead);
    QString fileName = fileToRead.fileName();
    QFileInfo info(fileName);
    QString file(info.fileName());
    //int inde = file.indexOf('-');

    uint8_t subOff = 21;
    if (!file.contains("ciphertext")) {
        subOff -= 11;
    }
    QString firmVer = file.mid(6, file.length() - subOff);
    dataArr.resize(PACK_SIZE);
    for (int16_t i = 0; i < PACK_SIZE; i++)
        dataArr[i] = 0;

    if (fileToRead.open(QIODevice::ReadOnly)) {
        Filesize = fileToRead.size(); //when file does open.
    }

    if (Filesize == 0) {
        fileToRead.close();
        return;
    }
    uint16_t txIters = Filesize / (PACK_SIZE - 2);
    uint16_t txRem = Filesize % (PACK_SIZE - 2);
    ui->progressBar->setMaximum(txIters + (txRem ? 1 : 0) - 1);
    ui->progressBar->setValue(0);

    objSwifi.CommandHeader(0xA3, timestamp, dataArr, &ind);
    objSwifi.AddLong(0xff, &ind, dataArr, Filesize);
    objSwifi.CommandFooter(dataArr, &ind);

    QTcpSocket* _pSocket;
    _pSocket = new QTcpSocket(this); // <-- needs to be a member variable: QTcpSocket * _pSocket;
    _pSocket->connectToHost("192.168.1.1", 5001);
    if (_pSocket->waitForConnected(5000)) {
        _pSocket->write(dataArr.data(), ind); // SEND 1st command

        if (objSwifi.waitForACK(_pSocket, 23, 5) != 0) {
            messageBox.critical(0, "Error", "Transmission error has occurred. Please repeat the update process.");
            messageBox.setFixedSize(500, 200);
            fileToRead.close();
            _pSocket->close();
            return;
        }

        ind = 0;
        objSwifi.CommandHeader(0xA0, timestamp, dataArr, &ind);
        objSwifi.AddString(1, &ind, dataArr, firmVer.toStdString());
        objSwifi.CommandFooter(dataArr, &ind);

        _pSocket->write(dataArr.data(), ind); // SEND 2nd command

        if (objSwifi.waitForACK(_pSocket, 23, 45) != 0) {
            messageBox.critical(0, "Error", "Transmission error has occurred. Please repeat the update process.");
            messageBox.setFixedSize(500, 200);
            fileToRead.close();
            _pSocket->close();
            return;
        }

        if (objSwifi.waitForACK(_pSocket, 23, 45) != 0) {
            messageBox.critical(0, "Error", "Transmission error has occurred. Please repeat the update process.");
            messageBox.setFixedSize(500, 200);
            fileToRead.close();
            _pSocket->close();
            return;
        }

        /*
        * Transfer binary data
        */
        qint32 bytes;
        uint16_t crc;
        uint16_t crcInd = PACK_SIZE - 2;
        for (int i = 0; i < txIters; i++) {
            bytes = s.readRawData(dataArr.data(), (PACK_SIZE - 2));
            if (bytes != (PACK_SIZE - 2)) {
                return;
            }

            crc = objSwifi.GetCRC(dataArr);
            dataArr[crcInd] = (uint8_t)(crc >> 8);
            dataArr[crcInd + 1] = (uint8_t)(0xff & crc);

            _pSocket->write(dataArr.data(), PACK_SIZE);
            _pSocket->waitForReadyRead(15000);
            QThread::msleep(10);
            if (!i && _pSocket->bytesAvailable() != 23) {
                //qDebug() << "fail23 - 341" << _pSocket->bytesAvailable();
            } else if (i != 0 && _pSocket->bytesAvailable() != 16) {
                //qDebug() << "fail23 - 341" << _pSocket->bytesAvailable();
            }
            _pSocket->readAll();
            ui->progressBar->setValue(i);
        }

        /*
        * Transfer binary reminder data
        */
        if (txRem) {
            for (int16_t i = 0; i < PACK_SIZE; i++)
                dataArr[i] = 0;
            bytes = s.readRawData(dataArr.data(), txRem);
            if (bytes != txRem) {
                return;
            }
            crc = objSwifi.GetCRC(dataArr);
            dataArr[crcInd] = (uint8_t)(crc >> 8);
            dataArr[crcInd + 1] = (uint8_t)(0xff & crc);

            _pSocket->write(dataArr.data(), PACK_SIZE); // SEND 2nd command
            _pSocket->waitForReadyRead(5000);
            if (_pSocket->bytesAvailable() != 23) {
            }
            _pSocket->readAll(); // READ 1st command respond
            ui->progressBar->setValue(ui->progressBar->value() + 1);
        }
    }
    fileToRead.close();
    _pSocket->close();
}

void MainWindow::on_pushButton_Browse_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"), "", tr("Bin Files (*.bin)"));
    if (!fileNames.isEmpty()) {
        ui->lineEdit_Path->setText(fileNames.first());
    }
    QString pathName = ui->lineEdit_Path->text();
    QFile fileToRead(pathName);
    QString fileName = fileToRead.fileName();
    QFileInfo info(fileName);
    QString file(info.fileName());

    bool fileCorrect = file.contains("Pico2_");
    if (!fileCorrect) {
        fileCorrect = file.contains("pico2_");
    }
    if (!fileCorrect) {
        fileCorrect = file.contains("pico-v");
    }
    ui->pushButton->setEnabled(fileCorrect);
}
