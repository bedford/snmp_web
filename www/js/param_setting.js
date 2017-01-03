/* 网络参数设置相关js */
function display_network_param(network_param) {
	$("#mac_addr").val(network_param.mac_addr);
	$("#ip_addr").val(network_param.ip_addr);
	$("#gateway").val(network_param.gateway);
	$("#netmask").val(network_param.netmask);
	$("#master_dns").val(network_param.master_dns);
	$("#slave_dns").val(network_param.slave_dns);
}

function init_network_param() {
	var network_param = {
		"mac_addr" : "F0:FF:04:00:5D:F4",
		"ip_addr" : "192.168.0.100",
		"gateway" : "192.168.0.1",
		"netmask" : "255.255.255.0",
		"master_dns" : "8.8.8.8",
		"slave_dns" : ""
	};
	display_network_param(network_param);
}

/* 系统时间设置相关js */
function addzero(v) {if (v < 10) return '0' + v;return v.toString();}

function update_pc_time() {
	var date = new Date();
	var current_time = date.getFullYear().toString() + '-' + addzero(date.getMonth() + 1) + '-'
			+ addzero(date.getDate()) + ' ' + addzero(date.getHours()) + ':'
			+ addzero(date.getMinutes()) + ':' + addzero(date.getSeconds());
	$('#pc_time').val(current_time);
}

var interval_handle;

function init_timestamp() {
	var ntp_info = {
		"ntp_server_ip" : "192.168.0.100",
		"ntp_interval" : "60"
	};

	$('#ntp_server').val(ntp_info.ntp_server_ip);
	$('#ntp_interval').val(ntp_info.ntp_interval);
	update_pc_time();

	interval_handle = setInterval("update_pc_time()", 1000);
}

function checkLeave() {
	clearInterval(interval_handle);
}

/* SNMP设置相关js */
function load_snmp_param() {
	var snmp_param = {
		"snmp_unio" : "public",
		"trap_server_ip" : "192.168.0.100",
		"authority_ip" : [
			{
				"valid_flag" : "1",
				"ip" : "192.168.0.200"
			},
			{
				"valid_flag" : "1",
				"ip":"192.168.0.201"
			},
			{
				"valid_flag" : "0",
				"ip" : "192.168.0.202"
			},
			{
				"valid_flag" : "0",
				"ip":"192.168.0.203"
			},
		]
	};

	$('#trap_server_ip').val(snmp_param.trap_server_ip);
	$('#snmp_unio').val(snmp_param.snmp_unio);

	var html = "";
	for (var i in snmp_param.authority_ip) {
		if (snmp_param.authority_ip[i].valid_flag == "1") {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" onchange="update(\''+i+'\')" checked="checked"></span>';
		} else {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" onchange="update(\''+i+'\')"></span>';
		}
		html += '<input type="text" id="authority_ip_'+i+'" value="'+snmp_param.authority_ip[i].ip+'"></li>';
	}
	html += '<li class="snmp_item"><span class="fix_span">共同体</span><input type="text" id="snmp_unio">';
	html += '<input type="button" class="confirm_btn" id="snmp_unio_btn" value="确定" onclick="add_sms_contact()"></li>';
	$('#right_side').append(html);
}

function update(i) {
	
	var input_name = "\'#authority_ip_" + i + "\'";
	//$(input_name).attributes
}