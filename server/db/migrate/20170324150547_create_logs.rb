class CreateLogs < ActiveRecord::Migration
  def change
    create_table :logs do |t|
      t.text :body

      t.timestamps null: false
    end
  end
end
