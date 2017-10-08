
function addzero(v) {if (v < 10) return '0' + v;return v.toString();}

function load_timepicker_default() {
	var date = new Date();
	date.setHours(date.getHours() + 1);
	var end_time = date.getFullYear().toString() + '-' + addzero(date.getMonth() + 1) + '-'
			+ addzero(date.getDate()) + ' ' + addzero(date.getHours()) + ':'
			+ addzero(date.getMinutes()) + ':' + addzero(date.getSeconds());
	$('#end_time').val(end_time);

	date.setHours(date.getHours() - 2);
	var start_time = date.getFullYear().toString() + '-' + addzero(date.getMonth() + 1) + '-'
			+ addzero(date.getDate()) + ' ' + addzero(date.getHours()) + ':'
			+ addzero(date.getMinutes()) + ':' + addzero(date.getSeconds());
	$('#start_time').val(start_time);
}

function load_device_list(callback) {
	var json = new Object();
	json.msg_type = "query_data";
	json.cmd_type = "query_support_device";
	var data = $.toJSON(json);
	$.ajax({
	            url     : "/cgi-bin/common.cgi",
	            type    : "POST",
	            dataType: "json",
	            data    : data,
	            success : function(msg) {
					callback(msg);
	            },
	        });
}

function load_di_list(callback) {
    var json = new Object();
    json.msg_type = "query_data";
    json.cmd_type = "query_di_support";
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
					callback(msg);
                },
            });
}
