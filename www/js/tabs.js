/**
 * Created by Administrator on 2016/9/14.
 */
 /*(function($){
    $.fn.extend({
        h_tab:function(option){
            var defOpen = option && option.open || 'li:first-child',
                tabCons = $( '> div > div',this );
            //在这里引入this
            this.on( 'tabSelect', '> ul > li', function () {
                var $this = $( this ),
                    isSelect = $this.hasClass( 'select' ),
                    index = $this.index();
                if ( isSelect ) {
                    return false;
                }else {
                    $( this ).addClass( 'select' ).siblings( 'li' ).removeClass( 'select' );
                    tabCons.hide().eq( index ).show();
                }
            });
            this.on( 'click', '> ul > li', function ( event ) {
                event.preventDefault();
                $( this ).trigger( 'tabSelect' );
            });
            this.find( defOpen ).trigger('tabSelect');//触发自定义tabSelect事件
        }
    });
})(jQuery);
*/

 (function($){
    $.fn.extend({
        h_tab:function(option){
            var defOpen = option && option.open || 'li:first-child',
                tabCons = $( '> div > div',this );

            //在这里引入this
            this.on( 'tabSelect', '> ul > li', function () {
                var $this = $( this ),
				    $thisUrl = $('a', this).attr("href"),
					$iframe = $( '#content_iframe' ),
                    isSelect = $this.hasClass( 'select' ),
                    index = $this.index();
                if ( isSelect ) {
                    return false;
                }else {
                    $( this ).addClass( 'select' ).siblings( 'li' ).removeClass( 'select' );
                    tabCons.hide().eq( index ).show();
					$iframe.attr( "src", $thisUrl ); //更改iframe的src值；切换页面
                }
            });
            this.on( 'click', '> ul > li', function ( event ) {
                event.preventDefault();
                $( this ).trigger( 'tabSelect' );
            });
            this.find( defOpen ).trigger('tabSelect');//触发自定义tabSelect事件
        }
    });
})(jQuery);