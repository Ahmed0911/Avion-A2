C:
cd C:\Program Files\FlightGear 3.4.0

SET FG_ROOT=C:\Program Files\FlightGear 3.4.0\data
.\\bin\fgfs --aircraft=Kiki --fdm=null --native-fdm=socket,in,30,127.0.0.1,5502,udp --native-ctrls=socket,out,30,127.0.0.1,5505,udp --fog-fastest --disable-clouds --start-date-lat=2015:06:01:09:00:00 --disable-sound --in-air --enable-freeze --airport=LOWI --altitude=5000 --heading=0 --offset-distance=0 --offset-azimuth=0
