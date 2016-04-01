function getCookie(name) {
	var prefix = name + "=";
	var cookieStartIndex = document.cookie.indexOf(prefix);
	if (cookieStartIndex == -1)
		return null;
	var cookieEndIndex = document.cookie.indexOf(";", cookieStartIndex
			+ prefix.length);
	if (cookieEndIndex == -1)
		cookieEndIndex = document.cookie.length;
	return unescape(document.cookie.substring(cookieStartIndex + prefix.length,
			cookieEndIndex));
}
function setCookie(name, value) {
	document.cookie = name + "=" + escape(value) + "; path=/";
}
function setCookieElem(id, defv) {
	var val = getCookie(id);
	if (val == null) {
		val = defv;
		setCookie(id, val);
	}
	var f = document.getElementById(id);
	if(f.type === "checkbox") {
		f.checked = val == 1 ? true : false;
	} else {
		f.value = val;
	}
}
function updCookie(id) {
	var val;
	var f = document.getElementById(id);
	if(f.type === "checkbox") {
		val = f.checked ? 1 : 0;
	} else {
		val = f.value;
	}
	setCookie(id, val);
}
