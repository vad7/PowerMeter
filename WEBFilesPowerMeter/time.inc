<script type="text/javascript">
var x = ~sntp_time~*1000;
if(x){
var d = new Date(x);
document.getElementById('sntptime').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
}
var x2 = ~TotalCntTime~*1000;
if(x2){
var d2 = new Date(x2);
document.getElementById('timecnt').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
}
var d = new Date(new Date()-(~sys_mactime~/1000));
document.getElementById('starttime').innerHTML= d.toLocaleString();
</script>