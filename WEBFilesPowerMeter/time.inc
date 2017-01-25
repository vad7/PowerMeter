<script type="text/javascript">
var x, d;
x = ~sntp_time~*1000;
if(x){
d = new Date(x);
document.getElementById('sntptime').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
}
x = ~TotalCntTime~*1000;
if(x){
d = new Date(x);
document.getElementById('timecnt').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
}
d = new Date(new Date()-(~sys_mactime~/1000));
document.getElementById('starttime').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
</script>