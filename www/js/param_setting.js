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
    var json = new Object();
    json.msg_type = 0;
    json.cmd_type = 0;
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    display_network_param(msg);
                },
            });
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

function display_ntp_param(ntp_info)
{
	$('#ntp_server').val(ntp_info.ntp_server_ip);
	$('#ntp_interval').val(ntp_info.ntp_interval);
	update_pc_time();

	interval_handle = setInterval("update_pc_time()", 1000);
}

function init_timestamp() {
    var json = new Object();
    json.msg_type = 0;
    json.cmd_type = 3;
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    display_ntp_param(msg);
                },
            });
}

function checkLeave() {
	clearInterval(interval_handle);
}

/* SNMP设置相关js */
function display_snmp_param(snmp_param) {
	var html = "";
	for (var i in snmp_param.authority_ip) {
		if (snmp_param.authority_ip[i].valid_flag == "1") {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" onchange="update(\''
                +i+'\')" checked="checked"></span>';
		} else {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" onchange="update(\''
                +i+'\')"></span>';
		}
		html += '<input type="text" id="authority_ip_'+i+'" value="'+snmp_param.authority_ip[i].ip+'"></li>';
	}
	html += '<li class="snmp_item"><span class="fix_span">共同体</span><input type="text" id="snmp_union">';
	html += '<input type="button" class="confirm_btn" id="snmp_union_btn" value="确定" onclick="add_sms_contact()"></li>';
	$('#right_side').append(html);

	$('#trap_server_ip').val(snmp_param.trap_server_ip);
	$('#snmp_union').val(snmp_param.snmp_union);
}

function load_snmp_param() {
    var json = new Object();
    json.msg_type = 0;
    json.cmd_type = 1;
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    display_snmp_param(msg);
                },
            });
}

function update(i) {
	var input_name = "\'#authority_ip_" + i + "\'";
	//$(input_name).attributes
}

