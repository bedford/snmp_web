function data_query() {
	var query_result = [
	{
		"id":"100",
		"date":"2016-12-15 12:00:01",
		"device_number":"100",
		"device_name":"UPS",
		"param_name":"电压",
		"anolog_value":"221.3",
		"digital_value":"-"
	},
	{
		"id":"101",
		"date":"2016-12-15 12:05:01",
		"device_number":"100",
		"device_name":"UPS",
		"param_name":"电压",
		"anolog_value":"221.3",
		"digital_value":"-"
	}
	];
	data_load(query_result);
}

function data_load(param_list) {
	var html="";
	$.each(param_list, function(i, item) {
		html+="<tr>";
		html+="<td>"+item.date+"</td>";
		html+="<td>"+item.device_number+"</td>";
		html+="<td>"+item.device_name+"</td>";
		html+="<td>"+item.param_name+"</td>";
		html+="<td>"+item.anolog_value+"</td>";
		html+="<td>"+item.digital_value+"</td>";
		html+="</tr>"
	});
	$("#sms_table").append(html);
}

function addzero(v) {if (v < 10) return '0' + v;return v.toString();}

function load_timepicker_default() {
	var date = new Date();
	var end_time = date.getFullYear().toString() + '-' + addzero(date.getMonth() + 1) + '-'
			+ addzero(date.getDate()) + ' ' + addzero(date.getHours()) + ':'
			+ addzero(date.getMinutes()) + ':' + addzero(date.getSeconds());
	$('#end_time').val(end_time);

	date.setHours(date.getHours() - 1);
	var start_time = date.getFullYear().toString() + '-' + addzero(date.getMonth() + 1) + '-'
			+ addzero(date.getDate()) + ' ' + addzero(date.getHours()) + ':'
			+ addzero(date.getMinutes()) + ':' + addzero(date.getSeconds());
	$('#start_time').val(start_time);
}


/* sms查询及加载相关js */
function sms_sent_query() {
	var query_result = [
	{
		"alarm_time":"2016-12-15 12:00:01",
		"send_time":"2016-12-15 12:00:10",
		"contact_name":"张三",
		"phone_number":"13828891024",
		"message":"UPS电压值235伏，高于报警值",
		"send_result":"是"
	},
	{
		"alarm_time":"2016-12-16 07:05:01",
		"send_time":"2016-12-16 07:05:14",
		"contact_name":"张三",
		"phone_number":"13828891024",
		"message":"UPS电压值235伏，高于报警值",
		"send_result":"否"
	}
	];
	load_sms_sent(query_result);
}

function load_sms_sent(param_list) {
	var html="";
	$.each(param_list, function(i, item) {
		html+="<tr>";
		html+="<td>"+item.alarm_time+"</td>";
		html+="<td>"+item.send_time+"</td>";
		html+="<td>"+item.contact_name+"</td>";
		html+="<td>"+item.phone_number+"</td>";
		html+="<td>"+item.message+"</td>";
		html+="<td>"+item.send_result+"</td>";
		html+="</tr>"
	});
	$("#sms_table").append(html);
}

/* email相关操作js*/
function email_sent_query() {
	var query_result = [
	{
		"alarm_time":"2016-12-15 12:00:01",
		"send_time":"2016-12-15 12:00:10",
		"contact_name":"张三",
		"email":"13828891024@qq.com",
		"message":"UPS电压值235伏，高于报警值",
		"send_result":"是"
	},
	{
		"alarm_time":"2016-12-16 07:05:01",
		"send_time":"2016-12-16 07:05:14",
		"contact_name":"张三",
		"email":"blue@qq.com",
		"message":"UPS电压值235伏，高于报警值",
		"send_result":"否"
	}
	];
	load_email_sent(query_result);
}

function load_email_sent(param_list) {
	var html="";
	$.each(param_list, function(i, item) {
		html+="<tr>";
		html+="<td>"+item.alarm_time+"</td>";
		html+="<td>"+item.send_time+"</td>";
		html+="<td>"+item.contact_name+"</td>";
		html+="<td>"+item.email+"</td>";
		html+="<td>"+item.message+"</td>";
		html+="<td>"+item.send_result+"</td>";
		html+="</tr>"
	});
	$("#email_table").append(html);
}

/* 报警相关js */
function alarm_query() {
	var query_result = [
	{
		"id":"100",
		"date":"2016-12-15 12:00:01",
		"device_number":"100",
		"device_name":"UPS",
		"param_name":"电压",
		"anolog_value":"241.2",
		"alarm_value":"230"
	},
	{
		"id":"101",
		"date":"2016-12-15 12:05:01",
		"device_number":"100",
		"device_name":"UPS",
		"param_name":"电压",
		"anolog_value":"239.0",
		"alarm_value":"230"
	}
	];
	alarm_load(query_result);
}

function alarm_load(param_list) {
	var html="";
	$.each(param_list, function(i, item) {
		html+="<tr>";
		html+="<td>"+item.date+"</td>";
		html+="<td>"+item.device_number+"</td>";
		html+="<td>"+item.device_name+"</td>";
		html+="<td>"+item.param_name+"</td>";
		html+="<td>"+item.anolog_value+"</td>";
		html+="<td>"+item.alarm_value+"</td>";
		html+="</tr>"
	});
	$("#alarm_table").append(html);
}
