Customize and Build the Windows Installer
=========================================

Translations
------------

Most of the translations for the installer are taken directly from the
installer generator Inno Setup (http://www.jrsoftware.org/isinfo.php).

You can add translations for QClip-specific messages in
``shared/qclip.iss``. Just copy lines starting with ``en.`` from
``[Custom Messages]`` section and change prefix to ``de.`` (for german
translation).

Modify and Test Installation
----------------------------

Normally the installation file is generated automatically by CI
which creates a portable app folder from build files and runs Inno Setup.

You don't have to build the app again, you just need:

1. Download the unzipped portable version of the app.
2. Clone of this repository.
3. Install `Inno Setup <http://www.jrsoftware.org/isinfo.php>`__.
4. Open
   `shared/qclip.iss <https://github.com/zerovoxxx/QClip/blob/master/shared/qclip.iss>`__
   in Inno Setup and add few lines at the beginning of the file.

::

    #define AppVersion 2.8.1-beta
    #define Source C:\path\to\QClip-repository-clone
    #define Destination C:\path\to\QClip-portable

You should now be able to modify the file in Inno Setup and run it
easily.
