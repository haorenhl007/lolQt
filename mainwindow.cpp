// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag.
// All rights reserved.

#include <QMessageBox>
#include <QMovie>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QMediaResource>
#include <QAudioFormat>
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QAudioProbe>
#include <QThread>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QVector>
#include <QTime>
#include <QtCore/QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsform.h"
#include "consolewidget.h"
#include "wavewidget.h"
#include "energywidget.h"

class MainWindowPrivate
{
public:
    MainWindowPrivate()
        : settingsForm(new SettingsForm)
        , imageWidget(new ImageWidget)
        , consoleWidget(new ConsoleWidget)
        , waveWidget(new WaveWidget)
        , energyWidget(new EnergyWidget)
        , process(nullptr)
        , movie(new QMovie)
        , audio(new QMediaPlayer)
        , audioDecoder(0)
        , probe(new QAudioProbe)
        , originalFPS(0)
        , fps(0)
        , framesNeeded(0)
        , beatCount(0)
        , frameFilenamePattern("%1-%2.png")
    {
        beatSamplingTime.start();
        beatInterval.start();
        probe->setSource(audio);
    }

    SettingsForm *settingsForm;
    ImageWidget *imageWidget;
    ConsoleWidget *consoleWidget;
    WaveWidget *waveWidget;
    EnergyWidget *energyWidget;
    QProcess *process;
    QMovie *movie;
    QMediaPlayer *audio;
    QAudioDecoder *audioDecoder;
    QAudioProbe *probe;
    SampleBuffer samples;
    QString audioFilename;
    QString artist;
    QString title;
    QStringList tmpImageFiles;
    qreal originalFPS;
    qreal fps;
    int framesNeeded;
    QTime beatInterval;
    QTime beatSamplingTime;
    int beatCount;
    // %1 original filename
    // %2 sequence number (4 digits)
    QString frameFilenamePattern;

    ~MainWindowPrivate()
    {
        delete movie;
        delete audio;
        delete audioDecoder;
        delete probe;
    }
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);
    ui->setupUi(this);

    setWindowTitle(AppName);

    QSettings::setDefaultFormat(QSettings::NativeFormat);

    QHBoxLayout *hbox1 = new QHBoxLayout;
    hbox1->addWidget(d->imageWidget);
    ui->originalGroupBox->setLayout(hbox1);

    ui->horizontalLayout2->addWidget(d->waveWidget);
    QObject::connect(d->waveWidget, SIGNAL(analysisCompleted()), SLOT(analysisCompleted()));

    ui->horizontalLayout2->addWidget(d->energyWidget);

    QObject::connect(ui->actionOpenImage, SIGNAL(triggered()), SLOT(openImage()));
    QObject::connect(ui->actionOpenAudio, SIGNAL(triggered()), SLOT(openAudio()));
    QObject::connect(ui->actionSaveFrames, SIGNAL(triggered()), SLOT(onSaveCancelClicked()));
    QObject::connect(ui->actionSaveFramesAs, SIGNAL(triggered()), SLOT(saveVideoAs()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), SLOT(about()));
    QObject::connect(ui->actionSettings, SIGNAL(triggered()), SLOT(showSettings()));
    QObject::connect(ui->actionViewConsole, SIGNAL(toggled(bool)), SLOT(showConsole(bool)));
    QObject::connect(ui->saveFramesButton, SIGNAL(clicked()), SLOT(onSaveCancelClicked()));
    QObject::connect(ui->beatMePushButton, SIGNAL(clicked()), SLOT(countBeat()));
    QObject::connect(d->imageWidget, SIGNAL(gifDropped(QString)), SLOT(analyzeMovie(QString)));
    QObject::connect(d->imageWidget, SIGNAL(musicDropped(QString)), SLOT(analyzeAudio(QString)));
    QObject::connect(d->consoleWidget, SIGNAL(closed()), SLOT(consoleClosed()));
    QObject::connect(d->audio, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    QObject::connect(d->audio, SIGNAL(durationChanged(qint64)), d->waveWidget, SLOT(setDuration(qint64)));
    QObject::connect(d->audio, SIGNAL(positionChanged(qint64)), d->waveWidget, SLOT(setPosition(qint64)));
    QObject::connect(d->audio, SIGNAL(durationChanged(qint64)), d->energyWidget, SLOT(setDuration(qint64)));
    QObject::connect(d->audio, SIGNAL(positionChanged(qint64)), d->energyWidget, SLOT(setPosition(qint64)));
    QObject::connect(d->audio, SIGNAL(metaDataAvailableChanged(bool)), SLOT(metaDataAvailableChanged(bool)));
    QObject::connect(d->probe, SIGNAL(audioBufferProbed(QAudioBuffer)), SLOT(audioBufferReady(QAudioBuffer)));
    QObject::connect(ui->bpmSpinBox, SIGNAL(valueChanged(double)), SLOT(bpmChanged(double)));

    QObject::connect(d->audio, SIGNAL(volumeChanged(int)), ui->volumeDial, SLOT(setValue(int)));
    QObject::connect(ui->volumeDial, SIGNAL(valueChanged(int)), d->audio, SLOT(setVolume(int)));

    QObject::connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    restoreAppSettings();
    disableSave();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *e)
{
    Q_D(MainWindow);
    if (!processAllowedToBeCanceled())
        return e->ignore();
    if (d->process != nullptr)
        deleteProcess();
    saveAppSettings();
    cancelAudioAnalysis();
    removeTemporaryFiles();
    d->settingsForm->close();
    d->consoleWidget->close();
    e->accept();
}


void MainWindow::cancelAudioAnalysis(void)
{
    Q_D(MainWindow);
    d->waveWidget->cancel();
    d->energyWidget->cancel();
}


void MainWindow::openImage(void)
{
    Q_D(MainWindow);
    const QString &fileName =
            QFileDialog::getOpenFileName(
                this,
                tr("Open animated GIF"),
                d->settingsForm->getOpenDirectory(),
                tr("Animated GIF images (*.gif)"));
    if (fileName.isEmpty())
        return;
    QFileInfo fi(fileName);
    d->settingsForm->setOpenDirectory(fi.absoluteFilePath());
    analyzeMovie(fileName);
}


void MainWindow::openAudio(void)
{
    Q_D(MainWindow);
    const QString &fileName =
            QFileDialog::getOpenFileName(
                this,
                tr("Open audio file"),
                d->settingsForm->getOpenDirectory(),
                tr("Audio files (*.mp3 *.m4a)"));
    if (fileName.isEmpty())
        return;
    QFileInfo fi(fileName);
    d->settingsForm->setOpenDirectory(fi.absoluteFilePath());
    analyzeAudio(fileName);
}


void MainWindow::saveVideoAs(void)
{
    Q_D(MainWindow);
    bool success = d->settingsForm->chooseOutputFile();
    if (success)
        onSaveCancelClicked();
}


int gcd(int a, int b) {
    int tmp;
    while (a != 0) {
        tmp = a;
        a = b % a;
        b = tmp;
    }
    return b;
}


void MainWindow::onSaveCancelClicked(void)
{
    Q_D(MainWindow);
    if (d->process == nullptr)
        createProcess();
    switch(d->process->state()) {
    case QProcess::NotRunning:
    {
        saveVideo();
        break;
    }
    case QProcess::Starting:
    case QProcess::Running:
    default:
        cancelEncoding();
        break;
    }
}


void MainWindow::cancelEncoding(void)
{
    Q_D(MainWindow);
    ui->statusBar->showMessage(tr("Encoding canceled."), 3000);
    d->process->kill();
    deleteProcess();
    enableSave();
}


void MainWindow::saveVideo(void)
{
    Q_D(MainWindow);
    QFileInfo fi(d->settingsForm->getOutputFile());
    if (fi.exists()) {
        QMessageBox::StandardButton button;
        button = QMessageBox::question(
                    this,
                    tr("Overwrite file?"),
                    tr("The selected output file already exists."
                       " Do you want to overwrite the file?"));
        if (button != QMessageBox::Yes)
            return;
    }
    bool addMusicInfoAsSubtitle = false;
    if (d->settingsForm->getSubtitlesEnabled()) {
        if (!d->artist.isEmpty() && !d->title.isEmpty() && d->audio->duration() > 11 * 1000) {
            QFile subtitleFile(this->getSubtitleFilename());
            const QTime &subStartTime = QTime::fromMSecsSinceStartOfDay(d->audio->duration() - 11 * 1000);
            const QTime &subEndTime = QTime::fromMSecsSinceStartOfDay(d->audio->duration()- 1 * 1000);
            if (subtitleFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                subtitleFile.write("1\n");
                subtitleFile.write(
                            QString("%1:%2:%3,000 --> %4:%5:%6,000\n")
                            .arg(subStartTime.hour(), 2, 10, QChar('0'))
                            .arg(subStartTime.minute(), 2, 10, QChar('0'))
                            .arg(subStartTime.second(), 2, 10, QChar('0'))
                            .arg(subEndTime.hour(), 2, 10, QChar('0'))
                            .arg(subEndTime.minute(), 2, 10, QChar('0'))
                            .arg(subEndTime.second(), 2, 10, QChar('0'))
                            .toLocal8Bit());
                subtitleFile.write(tr("Music: %1 - %2")
                                   .arg(d->artist)
                                   .arg(d->title).toLocal8Bit());
                subtitleFile.close();
            }
            addMusicInfoAsSubtitle = true;
        }
    }
    QFile frameFileList(this->getFrameFileListFilename());
    if (frameFileList.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const int N = d->tmpImageFiles.count();
        const int frameOffset = ui->offsetSpinBox->value();
        for (int i = 0; i < d->framesNeeded; ++i) {
            frameFileList.write(d->tmpImageFiles[(i + frameOffset) % N].toLocal8Bit());
            frameFileList.write("\n");
        }
        frameFileList.close();
    }
    const QImage &frame = d->movie->currentImage();
    int aspectDiv = gcd(frame.width(), frame.height());
    QString cmdLine =
            QString("\"%1\"")
            .arg(d->settingsForm->getMencoderPath()) +
            QString(" mf://@%1 ")
            .arg(frameFileList.fileName());
    QString mOpts = d->settingsForm->getMEncoderOptions();
    mOpts.replace("%1", QString::number(frame.width()));
    mOpts.replace("%2", QString::number(frame.height()));
    mOpts.replace("%3", QString::number(d->fps));
    mOpts.replace("%4", QString::number(frame.width() / aspectDiv));
    mOpts.replace("%5", QString::number(frame.height() / aspectDiv));
    mOpts.replace("%6", QString::number(d->settingsForm->getAudioBitrate()));
    mOpts.replace("%7", QString::number(QThread::idealThreadCount()));
    cmdLine += mOpts +
            QString(" -audiofile \"%1\"")
            .arg(d->audioFilename) +
            QString(" -o \"%1\"")
            .arg(d->settingsForm->getOutputFile());
    if (addMusicInfoAsSubtitle)
        cmdLine +=
                QString(" -sub \"%1\"")
                .arg(this->getSubtitleFilename()) +
                QString(" -font \"%1\"")
                .arg(d->settingsForm->getSubtitleFont()) +
                QString(" -subfont-text-scale 3");
    disableSave();
    d->consoleWidget->clear();
    d->consoleWidget->show();
    d->consoleWidget->out(cmdLine);
    d->process->start(cmdLine);
}


void MainWindow::createProcess(void)
{
    Q_D(MainWindow);
    d->process = new QProcess;
    QObject::connect(d->process, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
    QObject::connect(d->process, SIGNAL(readyReadStandardError()), this, SLOT(processErrorOutput()));
    QObject::connect(d->process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
}


void MainWindow::deleteProcess(void)
{
    Q_D(MainWindow);
    Q_ASSERT(d->process != nullptr);
    QObject::disconnect(d->process, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
    QObject::disconnect(d->process, SIGNAL(readyReadStandardError()), this, SLOT(processErrorOutput()));
    QObject::disconnect(d->process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
    d->process->waitForFinished();
    delete d->process;
    d->process = nullptr;
    // d->consoleWidget->hide();
}


bool MainWindow::processAllowedToBeCanceled(void)
{
    Q_D(MainWindow);
    if (d->process != nullptr && d->process->state() == QProcess::Running) {
        QMessageBox::StandardButton button;
        button = QMessageBox::question(
                    this,
                    tr("Really exit?"),
                    tr("The encoder is running in the background."
                       " If you quit, the process will be cancelled and the results be lost."
                       " Do you really want to quit?"));
        if (button == QMessageBox::Yes) {
            d->process->close();
            return true;
        }
        else {
            return false;
        }
    }
    return true;
}


QString MainWindow::getSubtitleFilename(void) const
{
    return d_ptr->settingsForm->getTempDirectory() + "/" + AppName + "-subtitles.srt";
}


QString MainWindow::getFrameFileListFilename(void) const
{
    return d_ptr->settingsForm->getTempDirectory() + "/" + AppName + "-list.txt";
}


void MainWindow::removeTemporaryFiles(void)
{
    Q_D(MainWindow);
    foreach (QString tmpImageFile, d->tmpImageFiles)
        QFile::remove(tmpImageFile);
    QFile::remove(this->getFrameFileListFilename());
    QFile::remove(this->getSubtitleFilename());
}


void MainWindow::processOutput(void)
{
    Q_D(MainWindow);
    const QByteArray &buf = d->process->readAllStandardOutput();
    ui->statusBar->showMessage(buf);
    d->consoleWidget->out(buf);
}


void MainWindow::processErrorOutput(void)
{
    Q_D(MainWindow);
    const QByteArray &buf = d->process->readAllStandardError();
    d->consoleWidget->out(buf);
}


void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_D(MainWindow);
    if (exitStatus == QProcess::NormalExit)
        ui->statusBar->showMessage(tr("Written video to \"%1\".").arg(d->settingsForm->getOutputFile()));
    else
        ui->statusBar->showMessage(tr("Warning! MEncoder exited unexpectedly. Video may not have been written."));
    enableSave();
    deleteProcess();
    removeTemporaryFiles();
}


void MainWindow::audioBufferReady(const QAudioBuffer &buf)
{
    Q_UNUSED(buf);
    // qDebug() << buf.startTime() << buf.duration() << buf.sampleCount();
}


void MainWindow::metaDataAvailableChanged(bool available)
{
    Q_D(MainWindow);
    // don't need to check availability because metaData() will
    // return an empty string if no metadata is available
    Q_UNUSED(available);
    d->artist = d->audio->metaData("Author").toString();
    d->title = d->audio->metaData("Title").toString();
}


void MainWindow::setVolume(void)
{
    Q_D(MainWindow);
    QAction *action = reinterpret_cast<QAction*>(sender());
    if (action)
        d->audio->setVolume(action->data().toInt());
}


void MainWindow::showConsole(bool show)
{
    Q_D(MainWindow);
    if (show)
        d->consoleWidget->show();
    else
        d->consoleWidget->hide();
}


void MainWindow::consoleClosed(void)
{
    ui->actionViewConsole->setChecked(false);
}


void MainWindow::calculateFPS(void)
{
    Q_D(MainWindow);
    if (d->movie->frameCount() > 0) {
        qreal bpm = ui->bpmSpinBox->value();
        d->fps = bpm / 60.0 * d->movie->frameCount();
        ui->fpsDoubleSpinBox->setValue(d->fps);
        d->movie->setSpeed(qRound(100 / d->originalFPS * d->fps));
        qreal duration = d->audio->duration() > 0 ? 1e-3 * d->audio->duration() : 1;
        d->framesNeeded = qRound(d->fps * duration);
    }
}


void MainWindow::durationChanged(qint64)
{
    calculateFPS();
}


void MainWindow::bpmChanged(double)
{
    calculateFPS();
}


void MainWindow::analyzeMovie(const QString &fileName)
{
    Q_D(MainWindow);
    d->movie->stop();
    d->movie->setFileName(fileName);
    QFileInfo fi(fileName);
    const int nFrames = d->movie->frameCount();
    if (d->movie->isValid() && nFrames > 0) {
        ui->offsetSpinBox->setMaximum(nFrames - 1);
        ui->offsetSpinBox->setValue(0);
        ui->offsetSpinBox->setEnabled(true);
        d->tmpImageFiles.clear();
        qreal duration = 0;
        d->movie->jumpToFrame(0);
        for (int i = 0; i < nFrames; ++i) {
            const QString &fqSavePath =
                    d->settingsForm->getTempDirectory() +
                    "/" +
                    d->frameFilenamePattern.arg(fi.baseName()).arg(i, 4, 10, QChar('0'));
            const QImage &frame = d->movie->currentImage();
            frame.save(fqSavePath);
            duration += d->movie->nextFrameDelay();
            d->tmpImageFiles.push_back(fqSavePath);
            d->movie->jumpToNextFrame();
        }
        d->originalFPS = 1e3 * qreal(nFrames) / duration;
        d->fps = d->originalFPS;
        ui->statusBar->showMessage(tr("%1 frames, %2 fps, %3 ms")
                                   .arg(nFrames)
                                   .arg(d->originalFPS, 0, 'g', 4)
                                   .arg(int(duration)), 3000);
        if (!d->audioFilename.isEmpty())
            enableSave();
        d->imageWidget->setMovie(d->movie);
        d->movie->start();
        calculateFPS();
    }
    else {
        disableSave();
        ui->offsetSpinBox->setEnabled(false);
    }
}


void MainWindow::analyzeAudio(const QString &fileName)
{
    Q_D(MainWindow);

    cancelAudioAnalysis();
    d->audioFilename = fileName;

    if (d->audioDecoder) {
        QObject::disconnect(d->audioDecoder, SIGNAL(bufferReady()));
        QObject::disconnect(d->audioDecoder, SIGNAL(finished()));
        delete d->audioDecoder;
    }
    d->audioDecoder = new QAudioDecoder;
    QObject::connect(d->audioDecoder, SIGNAL(bufferReady()), SLOT(readAudioBuffer()));
    QObject::connect(d->audioDecoder, SIGNAL(finished()), SLOT(finishedAudioBuffer()));

    d->samples.clear();

    d->audioDecoder->setSourceFilename(fileName);
    d->audioDecoder->start();

    d->audio->setMedia(QUrl::fromLocalFile(fileName));
    d->audio->play();

    ui->volumeDial->setEnabled(true);
    if (d->movie->isValid() && d->movie->frameCount() > 0)
        enableSave();
}


void MainWindow::readAudioBuffer(void)
{
    Q_D(MainWindow);
    const QAudioBuffer &buf = d->audioDecoder->read();
    if (!buf.isValid())
        return;
    if (buf.format().sampleSize() == 8 * sizeof(SampleBufferType)) {
        for (int i = 0; i < buf.sampleCount(); ++i)
            d->samples.append(buf.constData<SampleBufferType>()[i]);
        ui->statusBar->showMessage(tr("Decoding audio ... %1%")
                                   .arg(100LL * buf.startTime() / d->audioDecoder->duration()));
    }
}


void MainWindow::finishedAudioBuffer(void)
{
    Q_D(MainWindow);
    d->samples.squeeze();
    ui->statusBar->showMessage(tr("Analyzing audio ..."));
    d->waveWidget->setSamples(d->samples, d->audio->duration());
    d->energyWidget->setSamples(d->samples);
}


void MainWindow::analysisCompleted(void)
{
    ui->statusBar->showMessage(tr("Analysis complete."), 2000);
}



void MainWindow::countBeat(void)
{
    Q_D(MainWindow);
    if (d->beatInterval.elapsed() > 1 * 1000) {
        d->beatCount = 0;
        d->beatSamplingTime.start();
        d->beatInterval.start();
        ui->bpmSpinBox->setStyleSheet("background-color: transparent");
    }
    else {
        ++d->beatCount;
        if (d->beatSamplingTime.elapsed() > 0) {
            qreal bpm = 60e3 * d->beatCount / qreal(d->beatSamplingTime.elapsed());
            ui->bpmSpinBox->setValue(bpm);
            ui->bpmSpinBox->setStyleSheet("background-color: #ffcc33");
            d->beatInterval.start();
        }
    }
}


void MainWindow::showSettings(void)
{
    Q_D(MainWindow);
    d->settingsForm->show();
}


void MainWindow::saveAppSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
    settings.setValue("Settings/outputFile", d->settingsForm->getOutputFile());
    settings.setValue("Settings/openDir", d->settingsForm->getOpenDirectory());
    settings.setValue("Settings/tmpDir", d->settingsForm->getTempDirectory());
    settings.setValue("Settings/mencoderPath", d->settingsForm->getMencoderPath());
    settings.setValue("Settings/audioBitrate", d->settingsForm->getAudioBitrate());
    settings.setValue("Settings/mencoderOptions", d->settingsForm->getMEncoderOptions());
    settings.setValue("Settings/subtitleFont", d->settingsForm->getSubtitleFont());
    settings.setValue("Settings/volume", d->audio->volume());
    settings.setValue("Settings/frameOffset", ui->offsetSpinBox->value());
    settings.setValue("Console/geometry", d->consoleWidget->saveGeometry());
}


void MainWindow::restoreAppSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    d->consoleWidget->restoreGeometry(settings.value("Console/geometry").toByteArray());
    d->settingsForm->setOutputFile(settings.value("Settings/outputFile", d->settingsForm->getOutputFile()).toString());
    d->settingsForm->setOpenDirectory(settings.value("Settings/openDir", d->settingsForm->getOpenDirectory()).toString());
    d->settingsForm->setTempDirectory(settings.value("Settings/tmpDir", d->settingsForm->getTempDirectory()).toString());
    d->settingsForm->setMencoderPath(settings.value("Settings/mencoderPath", d->settingsForm->getMencoderPath()).toString());
    d->settingsForm->setMEncoderOptions(settings.value("Settings/mencoderOptions", d->settingsForm->getMEncoderOptions()).toString());
    d->settingsForm->setSubtitleFont(settings.value("Settings/subtitleFont", d->settingsForm->getSubtitleFont()).toString());
    d->settingsForm->setAudioBitrate(settings.value("Settings/audioBitrate", d->settingsForm->getAudioBitrate()).toInt());
    d->audio->setVolume(settings.value("Settings/volume", 50).toInt());
    ui->offsetSpinBox->setValue(settings.value("Settings/frameOffset", 0).toInt());
}


void MainWindow::about(void)
{
    QMessageBox::about(
                this,
                tr("About %1 %2").arg(AppName).arg(AppVersionNoDebug),
                tr("<p><b>%1</b> merges animated GIFs and music into videos. "
                   "See <a href=\"%2\" title=\"%1 project homepage\">%2</a> for more info.</p>"
                   "<p>Copyright &copy; 2014 %3 &lt;%4&gt;, Heise Zeitschriften Verlag.</p>"
                   "<p>This program is free software: you can redistribute it and/or modify "
                   "it under the terms of the GNU General Public License as published by "
                   "the Free Software Foundation, either version 3 of the License, or "
                   "(at your option) any later version.</p>"
                   "<p>This program is distributed in the hope that it will be useful, "
                   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
                   "GNU General Public License for more details.</p>"
                   "You should have received a copy of the GNU General Public License "
                   "along with this program. "
                   "If not, see <a href=\"http://www.gnu.org/licenses/gpl-3.0\">http://www.gnu.org/licenses</a>.</p>")
                .arg(AppName).arg(AppUrl).arg(AppAuthor).arg(AppAuthorMail));
}


void MainWindow::enableSave(void)
{
    ui->saveFramesButton->setText(tr("Save frames"));
    ui->saveFramesButton->setEnabled(true);
    ui->actionSaveFramesAs->setEnabled(true);
    ui->actionSaveFrames->setEnabled(true);
}


void MainWindow::disableSave(void)
{
    ui->saveFramesButton->setText(tr("Stop encoding"));
    ui->actionSaveFramesAs->setEnabled(false);
    ui->actionSaveFrames->setEnabled(false);
}
