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
    json.msg_type = "get_param";
    json.cmd_type = "network";
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
	json.msg_type = "set_param";
	json.cmd_type = "network";

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

/* SNMP设置相关js */
function set_snmp_param() {
	var json = new Object();
	json.msg_type = "set_param";
	json.cmd_type = "snmp";

	var cfg = new Object();
	var newArray = new Array();
	for (var i = 0; i < 4; i++) {
		var obj = new Object();
		if ($('#snmp_check_box_'+i).is(':checked')) {
			obj.valid_flag = 1;
		} else {
			obj.valid_flag = 0;
		}
		obj.ip = $('#authority_ip_'+i).val();
		newArray[i] = obj;
	}
	cfg.authority_ip = newArray;
	cfg.snmp_union = $('#snmp_union').val();
	cfg.trap_server_ip = $('#trap_server_ip').val();
	json.cfg = cfg;

	$('#set_snmp_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#set_snmp_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}

function display_snmp_param(snmp_param) {
	var html = "";
	for (var i in snmp_param.authority_ip) {
		if (snmp_param.authority_ip[i].valid_flag == 1) {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" id="snmp_check_box_'
				+i+'" checked="checked"></span>';
		} else {
			html += '<li class="snmp_item"><span class="fix_span"><input type="checkbox" id="snmp_check_box_'
                +i+'"></span>';
		}
		html += '<input type="text" id="authority_ip_'+i+'" value="'+snmp_param.authority_ip[i].ip+'"></li>';
	}
	html += '<li class="snmp_item"><span class="fix_span">共同体</span><input type="text" id="snmp_union">';
	html += '<input type="button" class="confirm_btn" id="set_snmp_btn" value="确定" onclick="set_snmp_param()"></li>';
	$('#right_side').append(html);

	$('#trap_server_ip').val(snmp_param.trap_server_ip);
	$('#snmp_union').val(snmp_param.snmp_union);
}

function load_snmp_param() {
    var json = new Object();
    json.msg_type = "get_param";
    json.cmd_type = "snmp";
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
	for (var i in io_param.support_list) {
	    $('#rs232_protocol_select').append('<option value="'+ io_param.support_list[i].protocol_id
			+ '">' + io_param.support_list[i].protocol_name + '</option>');
		$('#rs485_protocol_select').append('<option value="'+ io_param.support_list[i].protocol_id
			+ '">' + io_param.support_list[i].protocol_name + '</option>');
	}

	$('#rs232_protocol_select').val(io_param.rs232_cfg.protocol_id);
	$('#rs232_baudrate_select').val(io_param.rs232_cfg.baud);
	$('#rs232_databits').val(io_param.rs232_cfg.data_bits);
	$('#rs232_stopbits').val(io_param.rs232_cfg.stops_bits);
	$('#rs232_parity').val(io_param.rs232_cfg.parity);

	$('#rs485_protocol_select').val(io_param.rs485_cfg.protocol_id);
	$('#rs485_baudrate_select').val(io_param.rs485_cfg.baud);
	$('#rs485_databits').val(io_param.rs485_cfg.data_bits);
	$('#rs485_stopbits').val(io_param.rs485_cfg.stops_bits);
	$('#rs485_parity').val(io_param.rs485_cfg.parity);

	if (io_param.rs232_cfg.enable == 1) {
		$('#rs232_checkbox').attr("checked", true);
	}

	if (io_param.rs485_cfg.enable == 1) {
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
    json.msg_type = "get_param";
    json.cmd_type = "io";
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

function set_uart_param() {
	var json = new Object();
	json.msg_type = "set_param";
	json.cmd_type = "uart";

	var rs232_cfg = new Object();
	rs232_cfg.protocol_id = $('#rs232_protocol_select').val();
	if (rs232_cfg.protocol_id == null) {
		rs232_cfg.protocol_id = '0';
	}
	rs232_cfg.baud = $('#rs232_baudrate_select').val();
	rs232_cfg.data_bits = $('#rs232_databits').val();
	rs232_cfg.stops_bits = $('#rs232_stopbits').val();
	rs232_cfg.parity = $('#rs232_parity').val();
	if ($('#rs232_checkbox').is(':checked')) {
		rs232_cfg.enable = '1';
	} else {
		rs232_cfg.enable = '0';
	}

	var rs485_cfg = new Object();
	rs485_cfg.protocol_id = $('#rs485_protocol_select').val();
	if (rs485_cfg.protocol_id == null) {
		rs485_cfg.protocol_id = '0';
	}
	rs485_cfg.baud = $('#rs485_baudrate_select').val();
	rs485_cfg.data_bits = $('#rs485_databits').val();
	rs485_cfg.stops_bits = $('#rs485_stopbits').val();
	rs485_cfg.parity = $('#rs485_parity').val();
	if ($('#rs485_checkbox').is(':checked')) {
		rs485_cfg.enable = '1';
	} else {
		rs485_cfg.enable = '0';
	}

	var cfg = new Object();
	cfg.rs232_cfg = rs232_cfg;
	cfg.rs485_cfg = rs485_cfg;

	json.cfg = cfg;
	$('#confirm_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#confirm_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}
