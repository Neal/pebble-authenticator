# Authenticator

Authenticator is a [TOTP](http://en.wikipedia.org/wiki/Time-based_One-time_Password_Algorithm) based two-factor authentication manager for Pebble. It generates **T**ime-based **O**ne-**t**ime **P**assword for any service offering TOTP two-factor authentication including Google, Dropbox, Facebook, Microsoft, GitHub, Linode, etc.

This is a SDK 2.0 port of IEF's authenticator fork, which is a fork of pokey9000's twostep.

Option to vibrate when 5 seconds remain until the token renews and another one for new token.

Uses the JS:Configuration to set the timezone and enable vibration (look for the gear icon near Authenticator in the Pebble mobile app).

Adding secrets via JS:Configuration posseses a security risk of your tokens being sent through a remote server.

Will no longer need to set timezone manually once Pebble SDK supports timezone natively.

## Requirements

Running the `pebble` command assumes you have Pebble SDK 2.0 installed configured to compile Pebble apps.
More info on how to set that up found [here](https://developer.getpebble.com/2/getting-started/).

## Configuration

* Copy `configuration-sample.txt` to `configuration.txt` and add your secrets.
* Run `./configuration.py`

## Install

	$ pebble build
	$ pebble install
