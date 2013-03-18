// lobby client side script
// (C) Kristina Simpson 2013

function hideAddressBar(){
    if( document.documentElement.scrollHeight < window.outerHeight / window.devicePixelRatio ) {
        document.documentElement.style.height=(window.outerHeight/window.devicePixelRatio)+'px';
    }
    setTimeout( 'window.scrollTo( 1, 1 )', 0 );
}

// Sets up some things for mobile browers, stops the device scrolling, hides the address bar.
function configure_for_mobile() {
    hideAddressBar();
    $( window ).on( 'load', hideAddressBar() );
    $( window ).on( 'orientationchange', hideAddressBar() );
    
    // Prevent touches from scrolling window
    //$( 'body' ).on( 'touchmove', function(event) {
    //    event.preventDefault();
    //} );
}

function add_user_reqest(user, pass, email, avatar) {
    $.ajax( {
        url: '/user',
        dataType: 'json',
		type: 'POST',
		processData: false,
		data: JSON.stringify({
			'type': 'create_new_user',
			'user': user,
			'password': Sha1.hash(pass),
			'email': email,
			'avatar': avatar
		}),
		success: function( content, textStatus, jqXHR ) {
			if(content.type === 'error') {
				$( '#info_display' ).html( '<b>' + content.description + '</b>' );
			} else {
				$( '#info_display' ).html( '<b>User added.</b>' );
			}
		},
		error: function( jqXHR, textStatus, errorThrown ) {
			$( '#info_display' ).html( '<b>Error getting data: ' + textStatus + '</b>' );
			console.log( 'ajax error data: ' + textStatus );
		},
	} );
}

function init() {

	add_user_reqest('user1', 'pass', 'none@test.com', null);

	configure_for_mobile();
}

$( document ).ready( function() {
    init();
} );
