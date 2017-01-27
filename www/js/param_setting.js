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

function set_network_param() {
	var json = new Object();
	json.msg_type = 1;
	json.cmd_type = 0;

	var cfg = new Object();
	cfg.ip_addr = $("#ip_addr").val();
	cfg.gateway = $("#gateway").val();
	cfg.netmask = $("#netmask").val();
	cfg.master_dns = $("#master_dns").val();
	cfg.slave_dns = $("#slave_dns").val();
	json.cfg = cfg;

	$('#set_network_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#set_network_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}

/* 系统时间设置相关js */
function addzero(v) {
	if (v < 10) return '0' + v;
	return v.toString();
}

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
		if (snmp_param.authority_ip[i].valid_flag == 1) {
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

/* 串口及IO口状态及设置相关 */
$(function() {
	$('.do-status li').click(function() {
        var is_on = $(this).hasClass('do-status-on');
        if (is_on) {
            $(this).removeClass('do-status-on');
            $(this).addClass('do-status-off');
        } else {
            $(this).removeClass('do-status-off');
            $(this).addClass('do-status-on');
        }
    });
});

function display_io_param(io_param) {
    for (var i in io_param.uart_param) {
    	$('#rs232_baudrate_select').append('<option value="'+ io_param.uart_param[i].value
			+ '">' + io_param.uart_param[i].text + '</option>');
		$('#rs485_baudrate_select').append('<option value="'+ io_param.uart_param[i].value
			+ '">' + io_param.uart_param[i].text + '</option>');
	}

	for (var i in io_param.protocol_param) {
	    $('#rs232_protocol_select').append('<option value="'+ io_param.protocol_param[i].value
			+ '">' + io_param.protocol_param[i].text + '</option>');
		$('#rs485_protocol_select').append('<option value="'+ io_param.protocol_param[i].value
			+ '">' + io_param.protocol_param[i].text + '</option>');
	}

	$('#rs232_protocol_select').val(io_param.rs232_protocol);
	$('#rs232_baudrate_select').val(io_param.rs232_baudrate);

	$('#rs485_protocol_select').val(io_param.rs485_protocol);
	$('#rs485_baudrate_select').val(io_param.rs485_baudrate);

	if (io_param.rs232_flag) {
		$('#rs232_checkbox').attr("checked", true);
	}

	if (io_param.rs485_flag == 1) {
		$('#rs485_checkbox').attr("checked", true);
	}

	$('.do-status li').each(function(i) {
		if (io_param.io_status[i].value) {
            $(this).removeClass('do-status-off');
			$(this).addClass('do-status-on');
		}
	});
}

function load_io_param() {
    var json = new Object();
    json.msg_type = 0;
    json.cmd_type = 2;
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    display_io_param(msg);
                },
            });
}

function update(i) {
	var input_name = "\'#authority_ip_" + i + "\'";
	//$(input_name).attributes
}
