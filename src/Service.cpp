/****************************************************************************
**
** Copyright (C) 2016 Tobias Gläßer
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QResource>
#include <QSettings>
#include <QTranslator>
#include <QtDBus>

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <unistd.h>
#endif
#include <iostream>

#include "Utils.h"

#include "Run_Guard.h"

#include "Service.h"
#include "VM.h"
#include "main.h"

AQEMU_Service::AQEMU_Service()
{
  service = nullptr;
  main = nullptr;
  called_dbus = false;
  main_window = false;
  successful_init = false;
}

AQEMU_Service::~AQEMU_Service()
{
  delete service;
}

void AQEMU_Service::setMainWindow(bool b)
{
  main_window = b;
}

void AQEMU_Service::setMain(AQEMU_Main* m)
{
  main = m;
}

int AQEMU_Service::machineCount() const
{
  return machines.count();
}

void AQEMU_Service::vm_state_changed(Virtual_Machine* vm, VM::VM_State s)
{
  if (!QDBusConnection::sessionBus().isConnected()) {
    fprintf(stderr,
            "Cannot connect to the D-Bus session bus.\n"
            "To start it, run:\n"
            "\teval `dbus-launch --auto-syntax`\n");
  }
  else {
    QDBusInterface iface("org.aqemu.main_window", "/main_window", "",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
      /*QDBusReply<QString> reply =*/

      iface.call(QDBus::NoBlock, "VM_State_Changed", vm->Get_VM_XML_File_Path(),
                 s);

      /*if (reply.isValid()) {
                printf("Reply was: %s\n", qPrintable(reply.value()));*/
      //    return;
      /*}

            fprintf(stderr, "Call failed: %s\n", qPrintable(reply.error().message()));
            return;*/
    }
    else {
      fprintf(stderr, "%s\n",
              qPrintable(QDBusConnection::sessionBus().lastError().message()));
    }
  }

  //remove VM from service and shutdown the service, if there are no VMs running
  if (s == VM::VMS_Power_Off) {
    machines.removeAll(vm);
    //delete vm; //Segfaults //Destructor was already called at this point //Investigate how/why
  }
  if (machineCount() < 1) {
    if (!main_window) {
      QMetaObject::invokeMethod(QCoreApplication::instance(), "quit");
      //application->quit();
    }
  }
}

bool AQEMU_Service::isActive()
{
  return (machines.count() > 0);
}

bool AQEMU_Service::call(const QString& command, const QList<QVariant>& params,
                         bool noblock)
{
  called_dbus = true;

  if (init_service()) {
    if (main) {
      int ret = main->load_settings();
      if (ret != 0)
        return false;
    }

    successful_init = true;
  }

  if (!QDBusConnection::sessionBus().isConnected()) {
    fprintf(stderr,
            "Cannot connect to the D-Bus session bus.\n"
            "To start it, run:\n"
            "\teval `dbus-launch --auto-syntax`\n");
    return false;
  }

  QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
  if (iface.isValid()) {
    if (noblock) {
      iface.callWithArgumentList(QDBus::NoBlock, command, params);
    }
    else {
      QDBusReply<QString> reply =
          iface.callWithArgumentList(QDBus::AutoDetect, command, params);
      if (reply.isValid()) {
        std::cout << qPrintable(reply.value()) << std::endl;
        return true;
      }

      fprintf(stderr, "Call failed: %s\n", qPrintable(reply.error().message()));
      return false;
    }
  }

  fprintf(stderr, "%s\n",
          qPrintable(QDBusConnection::sessionBus().lastError().message()));
  return false;
}

bool AQEMU_Service::call(const QString& command, Virtual_Machine* vm,
                         bool noblock)
{
  return call(command, vm->Get_VM_XML_File_Path(), noblock);
}

bool AQEMU_Service::call(const QString& command, const QString& vm,
                         bool noblock)
{
  QList<QVariant> list;
  list << vm;
  return call(command, list, noblock);
}

bool AQEMU_Service::call(const QString& command, bool noblock)
{
  QList<QVariant> list;
  return call(command, list, noblock);
}

bool AQEMU_Service::call(const QString& command, Virtual_Machine* vm,
                         const QString& param2, bool noblock)
{
  QList<QVariant> list;
  list << vm->Get_VM_XML_File_Path();
  list << param2;
  return call(command, list, noblock);
}

bool AQEMU_Service::init_service()
{
  service =
      new Run_Guard("Gmp[0Ab6000");  //if service is already running, skip this
  if (service->tryToRun() == false) {
    delete service;
    service = nullptr;
    return false;
  }

  //dbus listening stuff

  if (!QDBusConnection::sessionBus().isConnected()) {
    fprintf(stderr,
            "Cannot connect to the D-Bus session bus.\n"
            "To start it, run:\n"
            "\teval `dbus-launch --auto-syntax`\n");
    return false;
  }

  if (!QDBusConnection::sessionBus().registerService(SERVICE_NAME)) {
    fprintf(stderr, "%s\n",
            qPrintable(QDBusConnection::sessionBus().lastError().message()));
    return false;
  }

  QDBusConnection::sessionBus().registerObject("/", this,
                                               QDBusConnection::ExportAllSlots);
  return true;
}

QString AQEMU_Service::start(const QString& s)
{
  QSettings settings;
  const auto& vm_dir = QDir::toNativeSeparators(
      settings.value("VM_Directory", QDir::homePath() + "/.aqemu/").toString());
  const auto& vm_file = vm_dir + s + ".aqemu";

  bool success = false;

  auto vm = new Virtual_Machine;
  if (QFileInfo(s).exists())
    success = vm->Load_VM(s);

  if (!success) {
    AQError("QString AQEMU_Service::start(const QString& s)", vm_file);

    if (QFileInfo(vm_file).exists())
      vm->Load_VM(vm_file);
    else {
      delete vm;
      return QString("VM \"%1\" could not be started. No such VM found.").arg(s);
    }
  }

  if (vm->Start()) {
    machines.append(vm);

    connect(vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)), this,
            SLOT(vm_state_changed(Virtual_Machine*, VM::VM_State)));

    AQError("QString AQEMU_Service::start(const QString& s)", s);
    return QString("VM \"%1\" got started.").arg(s);
  }

  return QString("VM \"%1\" could not be started.").arg(s);
}

Virtual_Machine* AQEMU_Service::getMachine(const QString& s)
{
  for (int i = 0; i < machines.count(); i++) {
    if (QFileInfo(machines.at(i)->Get_VM_XML_File_Path()) == QFileInfo(s) ||
        machines.at(i)->Get_Machine_Name() == s) {
      return machines.at(i);
    }
  }

  return nullptr;
}

QString AQEMU_Service::stop(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Stop();
    return QString("VM \"%1\" got stopped.").arg(s);
  }

  return QString("VM \"%1\" could not be stopped.").arg(s);
}

QString AQEMU_Service::shutdown(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Shutdown();
    return QString("shutting down VM \"%1\".").arg(s);
  }

  return QString("VM \"%1\" could not be shut down.").arg(s);
}

QString AQEMU_Service::reset(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Reset();
    return QString("VM \"%1\" got reset.").arg(s);
  }

  return QString("VM \"%1\" could  not be reset.").arg(s);
}

QString AQEMU_Service::pause(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Pause();
    return QString("VM \"%1\" got paused.").arg(s);
  }

  return QString("VM \"%1\" could not be paused.").arg(s);
}

QString AQEMU_Service::save(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Save_VM_State();
    return QString("VM state of \"%1\" got saved.").arg(s);
  }

  return QString("VM state of \"%1\" could not be saved.").arg(s);
}

QString AQEMU_Service::monitor(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Show_Monitor_Window();
    return QString("VM monitor window of \"%1\" got shown.").arg(s);
  }

  return QString("VM monitor window of \"%1\" could not be shown.").arg(s);
}

QString AQEMU_Service::error(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Show_Error_Log_Window();
    return QString("VM error log window of \"%1\" got shown.").arg(s);
  }

  return QString("VM error log window of \"%1\" could not be shown.").arg(s);
}

QString AQEMU_Service::control(const QString& s)
{
  if (auto machine = getMachine(s)) {
    machine->Show_Emu_Ctl_Win();
    return QString("VM control window of \"%1\" got shown.").arg(s);
  }

  return QString("VM control window of \"%1\" could not be shown.").arg(s);
}

QString AQEMU_Service::status(const QString& s)
{
  if (auto machine = getMachine(s)) {
    vm_state_changed(machine, machine->Get_State());
    QString state = machine->Get_State_Text();
    return QString("VM state:  %1.").arg(state);
  }
  else if (s.isEmpty()) {
    QString text;
    for (int i = 0; i < machines.count(); i++) {
      vm_state_changed(machines.at(i), machines.at(i)->Get_State());
      text += QString("VM \"%1\" state: %2.\n")
                  .arg(machines.at(i)->Get_Machine_Name(),
                       machines.at(i)->Get_State_Text());
    }

    if (!text.isEmpty())
      return text;
    else
      return QString("No VMs running.");
  }

  return QString("Could not show state of VM \"%1\".").arg(s);
}

QString AQEMU_Service::status()
{
  QString text;
  for (int i = 0; i < machines.count(); i++) {
    vm_state_changed(machines.at(i), machines.at(i)->Get_State());
    text += QString("VM \"%1\" state: %2.\n")
                .arg(machines.at(i)->Get_Machine_Name(),
                     machines.at(i)->Get_State_Text());
  }

  if (!text.isEmpty())
    return text;
  else
    return QString("No VMs running.");
}

QString AQEMU_Service::command(const QString& vm, const QString& command)
{
  if (auto machine = getMachine(vm)) {
    machine->Send_Emulator_Command(command);
    return QString("Sent command to VM \"%1\".").arg(vm);
  }

  return QString("Could not send command to VM \"%1\".").arg(vm);
}

QString AQEMU_Service::list()
{
  QSettings settings;
  QDir vm_dir(
      QDir::toNativeSeparators(settings.value("VM_Directory", "~").toString()));
  QFileInfoList file =
      vm_dir.entryInfoList(QStringList("*.aqemu"), QDir::Files, QDir::Name);

  if (file.count() <= 0)
    return "";

  QStringList list;
  for (int ix = 0; ix < file.count(); ix++)
    list << file[ix].filePath();

  //return names of all available machines
  return list.join("\n");
}
