/*
 * OTA Update Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

static const char rootFmt[] PROGMEM =R"(
<html>
  <head>
  </head>
  <body>
    <div style='font-size:250%%'>
      CL17 Chlorine Reading <b>%u</b>
      <form action="/send" method="GET">
        <input type="number" name="pumpSpeed" style="font-size:40px" value="%u"></input><br><br>
        <input type="submit"></input><br>
      </form>
      <input type="button" onclick="location='/config';" value="Config"></input>
    </div>
  </body>
</html>
)";

static const char configFmt[] PROGMEM =R"(
<html>
  <head>
  </head>
  <body>
    <div style='font-size:250%%'>
      <form action="/set" method="GET">
        SSID to join <input type="text" name="ssid" style="font-size:40px" value="%s"></input><br>
        Password for SSID to join <input type="text" name="pass" style="font-size:40px" value="%s"></input><br>
        SSID for Captive Net <input type="text" name="captive_ssid" style="font-size:40px" value="%s"></input><br>
        Password for Captive Net SSID <input type="text" name="captive_pass" style="font-size:40px" value="%s"></input><br>
        Spreading Factor (7-12) <input type="text" name="sf" style="font-size:40px" value="%i"></input><br>
        Timeout in seconds (0 for no timeout) <input type="text" name="timeout_val" style="font-size:40px" value="%i"></input><br>
        Number of timeouts before rebooting (0 for no rebooting) <input type="text" name="tt_reboot" style="font-size:40px" value="%i"></input><br>
        Monitor Mode <input type="checkbox" %s id="mmodeck" onchange="toggleMonMode();"><br>
        <input type="text" value="%s" name="monmode" id="monmode" hidden>
        <input type="submit"></input><br>
      </form>
      <input type="button" onclick="location='/reboot';" value="Reboot"></input><br>
      <input type="button" onclick="location='/ota';" value="OTA"></input>
    </div>
    <script>
      function toggleMonMode()
      {
        var cb=document.getElementById("mmodeck");
        document.getElementById("monmode").value=(cb.checked)?"true":"false";
      }
    </script>
  </body>
</html>
)";
