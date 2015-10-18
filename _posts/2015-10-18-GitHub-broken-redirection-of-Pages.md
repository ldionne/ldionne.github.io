---
layout: post
title: GitHub's broken redirection of Pages
comments: true
tags: GitHub Hana
---

I really love GitHub and their product makes my life much easier. Heck, I'm
even writing this post from [Atom](https://atom.io)! But their GitHub Pages
redirection is truly broken, and I need a solution. Here's my problem.

My [Hana](https://github.com/ldionne/hana) C++ metaprogramming library was
accepted into [Boost](https://boost.org). That's great, and I've been working
hard to polish some rough edges and stabilize the interface so it can be
shipped with the next Boost release and start being used in production. The
last step towards this goal is to integrate Hana with the rest of Boost, part
of which means transferring the ownership of the GitHub repository from me
([ldionne](https://github.com/ldionne)) to the Boost organization
([boostorg](https://github.com/boostorg)).

GitHub allows doing this very easily. Basically, you just go to your admin
panel and say _"transfer ownership to boostorg"_. When you press the button,
all the links, pushes, pulls and hooks are redirected to the new repository.
Great, right? Yes, except GitHub Pages sites are _not_ redirected. This
means that anyone going to [ldionne.github.io/hana](https://ldionne.github.io/hana)
will now get a 404 instead of being redirected to __boostorg.github.io/hana__,
as it ought to be. That's a real bummer, since I use GitHub Pages to host my
documentation, and the web contains two years worth of links to that
documentation.

So I basically have two choices. The first one is to use GitHub's broken
redirection and break all the links to my documentation. The other one is
to ditch GitHub's redirection by transferring the repository and then
overwriting the old repository with a new one saying _"this project has moved"_.
Unfortunately, this will disable any automatic redirection to the new repository,
thus breaking an unknown amount of things (e.g. scripts and existing clones),
but at least giving users an informative message instead of a 404.

Has anyone been through a similar dilemma? Why should I favor one option
over the other? If you have a solution, please let me know!

> By the way, I contacted GitHub support and they said they would consider
adding that feature. To me, that's a bug more than a missing feature.
Anyway, you can track the progression of the issue [here](https://github.com/isaacs/github/issues/492).
