Command Line
============

Tabs, items, clipboard and configuration can be changed through command
line interface. Run command ``qclip help`` to see complete list of
commands and their description.

.. warning::

    On Windows, you may not see any output when executing QClip in
    terminal/console (PowerShell or cmd).

    See workarounds in :ref:`known-issue-windows-console-output`.

To add new item to tab with name "notes" run:

::

    qclip tab notes add "This is the first note."

To print the item:

::

    qclip tab notes read 0

Add other item:

::

    qclip tab notes add "This is second note."

and print all items in the tab:

::

    qclip eval -- "tab('notes'); for(i=size(); i>0; --i) print(str(read(i-1)) + '\n');"

This will print:

::

    This is the first note.
    This is second note.

Among other things that are possible with QClip are:

* open video player if text copied in clipboard is URL with multimedia
* store text copied from a code editor in "code" tab
* store URLs in different tab
* save screenshots (print-screen)
* load all files from directory to items (create image gallery)
* replace a text in all matching items
* run item as a Python script
