Rails.application.routes.draw do
  resources :logs
  root to: 'visitors#index'
  devise_for :users
  post '/webhook', to: 'application#webhook'
end
