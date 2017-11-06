class ApplicationController < ActionController::Base
  # Prevent CSRF attacks by raising an exception.
  # For APIs, you may want to use :null_session instead.
  protect_from_forgery with: :exception

  skip_before_filter  :verify_authenticity_token, only: [:webhook]

  def webhook
  	@client = Twilio::REST::Client.new
  	if params["event"] == "C"
  		@client.messages.create(
			 from: '+16466797555',
			 to: '+19145751611',
			 body: 'Request from Noah for counseling at 914-575-1611'
		  )
  	elsif params["event"] == "G"
      @client.messages.create(
        from: '+16466797555',
        to: '+19145751611',
        body: 'Noah is in need of help here: https://www.google.com/maps?q=' + params[:data]
      )
  	end
	Log.create(body: params)
	head :ok, content_type: "text/html"
  end

end
