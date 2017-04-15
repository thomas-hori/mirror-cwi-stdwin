Complain about bad arguments (esp. window pointers) instead of ignoring
them

Don't scroll so much beyond end of window (in wshow?)

Textedit highlights disappear when they are redrawn after a menu covered
some text but *not* the highlight

Fix wreshuffle to minimize redrawing

Fix scrollupdate to minimize redrawing

Maintain a stack of active windows so active window after a delete is
less surprising

Don't grow windows beyond their document size

Let wmessage wait for CR to acknowledge?

Dreams:

Generalized key bindings read from a startup file

~ and $ expansion and file name completion in waskfile
(or write a tiny directory browser?)


Chages to stdwin by Euromath project?

draw.c		bogus! fix to scrollupdate;
		some comments added

stdwin.c	added wgetwinpos and wgetorigin;
		support for resize events;
		don't force showing the caret;
		don't reset the 'front' window when closing non-front;
		Lambert's attempts at imporving _wreshuffle???


Of the following changes I am not sure yet:

menu.c		changed standard menu title and contents;
		fixed minor bugs;
		and what else?  (lots changed)

scroll.c	only change is to comment out a fallback on wchange in
		wscroll when there is horizontal scrolling or not the
		entire window width is affected.  do we need this?

syswin.c	added wdialog which is like waskstr with an additional
		multiple-choice list of options (???) (why couldn't this
		be done as a normal stdwin window?);
		added RedrawAll
