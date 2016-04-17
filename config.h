// Smarthome Controller Config

// HUE CONFIG -----------------------

/* Bridge IP: Find this in the Hue smartphone app, in your
   router's config, or by visiting https://www.meethue.com/api/nupnp
*/
#define BRIDGE_IP "10.88.111.2"

/* This application needs to be registered as a user with your
   hue bridge. You only have to do this once:
   1. Visit http://<bridge ip address>/debug/clip.html in a browser.
   2. Put http://<bridge ip address>/api in the 'URL' field.
   3. Put {"devicetype":"smarthomecontroller newuser"} in the 'Message Body' field.
   4. Click the POST button, you will see an error 'link button not pressed'.
   5. Press the link button on your bridge. Click the POST button again.
   6. Enter the 'username' returned below.

   (similar steps at: http://www.developers.meethue.com/documentation/getting-started)
*/
#define USER "16e5a5ed363dea765934a93b18a8c3"
// ----------------------------------
