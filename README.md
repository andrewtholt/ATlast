ATLast
======

This is a fork of a fork.

It is a fork of Michael Henderson's (aka quoha) of Atlast.

I want to add a few features and use as the basis for a scriptable unit test framework.

I have added a ./build.sh script in atlast-1.2 (run ./build.sh -h for hints as to usage.)

27 August 2016 Now build with g++ and I have started to replace character io with a generic messaging class.  Current version behaves as before, ultimately will be able to have the console of IPC on Linux, and others.  Currently x86_64 only.

29 August This is now wholly in C++ and dependent on a set of classes from my Tools repo (Tools/Linux/C++/portableMessages

28 September 2016 Backed off a little from the above.  Console IO is through the class Console.  Removed the virtual class so it's better suited for embedded.

Below is Michael's original README.

=================

This is a fork of John Walker's ATLAST FORTH embeddable interpreter. My goal is to change the code to use a single
struct to hold the state of the interpreter. I think that will make it easier to embed. The cost will be speed. Going
through the struct will prevent many optimizations that were performed against the global variables. For my purposes,
this is an acceptable tradeoff.

You can find Mr. Walker's original work at https://www.fourmilab.ch/atlast/atlast.html. It has also been added to
other repositories here on GitHub. Searching for "atlast" will take you to them.
