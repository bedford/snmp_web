
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
